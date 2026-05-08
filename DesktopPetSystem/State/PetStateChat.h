#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include <QProcess>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>

struct ChatMessage {
    QString role;
    QString content;
};

class PetStateChat : public PetState
{
    Q_OBJECT

public:
    PetStateChat(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStateChat() override;

    /** 写入空摘要与空消息列表（配置文件中的 memory_path） */
    static void writeEmptyChatMemoryFile();
    /** 清空内存中的对话与摘要并写入磁盘；重置时若仍在对话状态须先调用，避免 exit() 写回旧记录 */
    void wipeChatTurnsSummaryAndSave();
    /** 从磁盘重新载入 chat_memory.json（手动编辑记忆文件后调用） */
    void reloadChatMemoryFromDisk();

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Chat; }
public slots:
    void onUserInput(const QString& input);
    void cancelCurrentChat();
    void onAiProcessStarted();
    void onProcessStdout();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

signals:
    void showChatWidget();
    void showChatReply(const QString& reply);
    void chatOllamaPrefetchFailed(const QString& message);
    void chatBusyChanged(bool busy);
    void chatAssistantStarted();
    void chatAssistantDelta(const QString& text);
    void chatAssistantFinished(bool success, bool userCancelled, const QString& errorMessage);

private:
    void finalizeAiReply(bool assistantBubbleWasShown,
                        bool success,
                        bool userCancelled,
                        const QString& errorMessage);

    void flushStdoutLines();
    void deliverChatStdinPayload();

    QString absoluteChatMemoryPath() const;
    void loadChatMemory();
    void saveChatMemory();
    void appendSuccessfulExchange(const QString& userText, const QString& assistantText);
    QJsonArray buildHistoryPayload() const;
    void tryCompressOldTurns();

    QJsonObject structuredMemoryToJson() const;
    void applyStructuredMemoryFromJson(const QJsonObject& o);
    void clearStructuredMemory();

    bool m_isChatting = false;
    QProcess* m_process = nullptr;
    QString m_currentInput;

    QByteArray m_stdoutLineBuffer;
    bool m_waitingForAiResponse = false;
    bool m_aiAssistantBubbleShown = false;
    bool m_userCancelled = false;
    bool m_finalizeDone = false;

    QVector<ChatMessage> m_chatMessages;
    QString m_memoryPreferences;
    QString m_memoryTasks;
    QString m_memoryAvoid;
    QString m_memoryFacts;
    bool m_summarizeInProgress = false;
    QString m_assistantStreamingBuffer;
    QString m_pendingDoneReply;
    QByteArray m_chatStdinPending;
};
