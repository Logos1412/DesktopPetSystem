#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QLabel>
#include <QPalette>

// 聊天界面组件
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget* parent = nullptr);
    ~ChatWidget() override = default;

    void showAtPos(const QPoint& pos);
    void addChatMessage(const QString& sender, const QString& message, bool isUser = false);
    /** 清空气泡并恢复初始欢迎语（与存档重置配合） */
    void resetConversationView();

public slots:
    void setChatGenerating(bool generating);
    /** 从 PetConfig 刷新占位符与字数上限（设置保存或 chat_runtime 同步后） */
    void reloadChatInputLimit();
    void beginAssistantBubble();
    void appendAssistantBubbleDelta(const QString& text);
    void endAssistantBubble(bool success, bool userCancelled, const QString& errorMessage);

signals:
    void userInput(const QString& input);
    void aiCancelRequested();
    void closed();

private slots:
    void updateInputCounterAndSendState();
    void onSendButtonClicked();
    void onInputReturnPressed();
    void onCancelButtonClicked();
    void onCloseButtonClicked();

protected:
    void focusOutEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#endif

private:
    void initUI();
    void initStyle();
    void initConnections();
    void scrollChatToBottom();

private:
    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_headerWidget = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_closeButton = nullptr;
    QScrollArea* m_chatScrollArea = nullptr;
    QWidget* m_chatContentWidget = nullptr;
    QVBoxLayout* m_chatLayout = nullptr;
    QLineEdit* m_inputEdit = nullptr;
    QPushButton* m_sendButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QLabel* m_inputCounter = nullptr;

    QLabel* m_streamContentLabel = nullptr;
    bool m_isGenerating = false;

    bool m_isDragging = false;
    QPoint m_dragStartPos;
};
