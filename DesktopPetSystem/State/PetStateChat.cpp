#pragma execution_character_set("utf-8")

#include "PetStateChat.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "Config/PetConfig.h"
#include <QDebug>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace {
QString resolveProjectRootPath()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        if (QFile::exists(dir.absoluteFilePath("resources/config/pet_config.json"))) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QCoreApplication::applicationDirPath();
}

QString absoluteChatMemoryPathFromConfig()
{
    PetConfig* cfg = PetConfig::getInstance();
    const QString rel = cfg->getChatMemoryPath();
    const QString root = resolveProjectRootPath();
    if (QDir::isRelativePath(rel)) {
        return QDir(root).absoluteFilePath(rel);
    }
    return rel;
}

bool queryOllamaTagsOk(const QString& hostBase, QString* detailOut)
{
    QString base = hostBase.trimmed();
    if (base.isEmpty()) {
        base = QStringLiteral("http://127.0.0.1:11434");
    }
    if (!base.endsWith(QLatin1Char('/'))) {
        base += QLatin1Char('/');
    }
    const QUrl url(base + QStringLiteral("api/tags"));

    QNetworkAccessManager nam;
    QNetworkRequest req(url);
    QNetworkReply* reply = nam.get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, [&]()
                     {
                         timer.stop();
                         loop.quit();
                     });
    timer.start(4000);
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (reply->isRunning()) {
        reply->abort();
        if (detailOut) {
            *detailOut = QStringLiteral("连接超时（约 4 秒）。请确认 Ollama 已启动并监听 11434。");
        }
        reply->deleteLater();
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (detailOut) {
            *detailOut = reply->errorString();
        }
        reply->deleteLater();
        return false;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200 && statusCode != 0) {
        if (detailOut) {
            *detailOut = QStringLiteral("HTTP %1").arg(statusCode);
        }
        reply->deleteLater();
        return false;
    }
    reply->deleteLater();
    return true;
}
}

PetStateChat::~PetStateChat()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
    }
}

void PetStateChat::enter()
{
    qDebug() << "进入聊天状态";

    QString errDetail;
    if (!queryOllamaTagsOk(PetConfig::getInstance()->getChatOllamaHost(), &errDetail)) {
        /* 先退回 Idle（避免未完成 enter）；提示用单次定时延后，绕过 changeState 重入期间的 UI 抖动 */
        m_fsm->changeState(PetStateType::Idle);
        const QString msg = QStringLiteral(
                                "请先启动 Ollama，或检查配置文件中的 chat_runtime.ollama_host。\n\n")
                            + errDetail.trimmed();
        QTimer::singleShot(0, this, [this, msg]() { emit chatOllamaPrefetchFailed(msg); });
        return;
    }

    m_isChatting = false;

    emit showChatWidget();
    emit requestPlayAnimation("chat/chat.gif", true);
    m_isChatting = true;
    loadChatMemory();
}

void PetStateChat::update()
{
    /* 聊天属性的饱食/精力/心情消耗已改为仅在 AI 一轮成功结束时结算（finalizeAiReply） */
}

void PetStateChat::exit()
{
    qDebug() << "退出聊天状态";
    m_isChatting = false;

    saveChatMemory();

    if (m_process) {
        disconnect(m_process, nullptr, this, nullptr);
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_stdoutLineBuffer.clear();
    m_chatStdinPending.clear();
    m_waitingForAiResponse = false;
    m_aiAssistantBubbleShown = false;
    m_userCancelled = false;
    m_finalizeDone = false;
}

void PetStateChat::finalizeAiReply(bool assistantBubbleWasShown,
                                 bool success,
                                 bool userCancelled,
                                 const QString& errorMessage)
{
    if (m_finalizeDone) {
        return;
    }
    m_finalizeDone = true;
    m_waitingForAiResponse = false;

    emit chatBusyChanged(false);

    QString assistantTextSaved;
    if (success && !userCancelled) {
        assistantTextSaved = m_assistantStreamingBuffer.trimmed();
        if (assistantTextSaved.isEmpty()) {
            assistantTextSaved = m_pendingDoneReply.trimmed();
        }
        if (assistantTextSaved.isEmpty()) {
            assistantTextSaved = QStringLiteral("\u2026");
        }
    }

    /* 先结束流式气泡 UI；避免在其后再做存档/摘要导致界面长时间停在「生成中」 */
    emit chatAssistantFinished(success, userCancelled, errorMessage);

    if (success && !userCancelled) {
        PetConfig* cfg = PetConfig::getInstance();
        const int costH = cfg->getChatHungerCostPerRound();
        const int costE = cfg->getChatEnergyCostPerRound();
        const int gainM = cfg->getChatMoodGainPerRound();
        const int h0 = m_attr->getHunger();
        const int e0 = m_attr->getEnergy();
        const int m0 = m_attr->getMood();

        m_attr->changeHunger(-costH);
        m_attr->changeEnergy(-costE);
        m_attr->changeMood(gainM);

        qDebug() << "[结算]" << "聊天" << "| 一轮 |"
                 << formatSettlementAttrDelta(QStringLiteral("饱食"), -costH)
                 << formatSettlementAttrDelta(QStringLiteral("精力"), -costE)
                 << formatSettlementAttrDelta(QStringLiteral("心情"), gainM)
                 << "| 结算前" << h0 << e0 << m0 << QStringLiteral("→ 结算后")
                 << m_attr->getHunger() << m_attr->getEnergy() << m_attr->getMood();

        appendSuccessfulExchange(m_currentInput, assistantTextSaved);
        saveChatMemory();
    }

    /* runSummarizeMerge 内含长时间 waitForFinished，只能在事件循环空闲时跑 */
    if (success && !userCancelled) {
        QTimer::singleShot(0, this, [this]() {
            tryCompressOldTurns();
            saveChatMemory();
        });
    }

    if (!assistantBubbleWasShown && !success && !userCancelled) {
        const QString fallback = errorMessage.isEmpty()
            ? QStringLiteral("抱歉，我暂时无法回应~")
            : errorMessage;
        emit showChatReply(fallback);
    }

    m_aiAssistantBubbleShown = false;
    m_userCancelled = false;
    m_assistantStreamingBuffer.clear();
    m_pendingDoneReply.clear();
}

void PetStateChat::flushStdoutLines()
{
    while (true) {
        int nl = m_stdoutLineBuffer.indexOf('\n');
        if (nl < 0) {
            break;
        }
        QByteArray lineBytes = m_stdoutLineBuffer.left(nl);
        m_stdoutLineBuffer.remove(0, nl + 1);

        QString line = QString::fromUtf8(lineBytes).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[聊天AI] 跳过无法解析的输出行";
            continue;
        }

        const QJsonObject obj = doc.object();
        const QString event = obj.value(QStringLiteral("event")).toString();

        if (event == QStringLiteral("delta")) {
            const QString delta = obj.value(QStringLiteral("content")).toString();
            if (!delta.isEmpty()) {
                m_assistantStreamingBuffer += delta;
                emit chatAssistantDelta(delta);
            }
        } else if (event == QStringLiteral("done")) {
            const bool success = obj.value(QStringLiteral("success")).toBool();
            QString errMsg = obj.value(QStringLiteral("error")).toString();
            const QString replyFallback = obj.value(QStringLiteral("reply")).toString();
            m_pendingDoneReply = replyFallback;
            const bool bubble = m_aiAssistantBubbleShown;
            if (!success && bubble && !replyFallback.isEmpty()) {
                m_assistantStreamingBuffer += replyFallback;
                emit chatAssistantDelta(replyFallback);
                errMsg.clear();
            }
            finalizeAiReply(bubble,
                            success,
                            false,
                            success ? QString() : (errMsg.isEmpty() ? QStringLiteral("生成失败") : errMsg));
        } else if (event.isEmpty() && obj.contains(QStringLiteral("reply"))) {
            /* 兼容旧版脚本：单行 JSON（无 event 字段） */
            const bool success = obj.value(QStringLiteral("success")).toBool(true);
            const QString reply = obj.value(QStringLiteral("reply")).toString();
            const QString errMsg = obj.value(QStringLiteral("error")).toString();
            m_pendingDoneReply = reply;
            const bool bubble = m_aiAssistantBubbleShown;
            if (bubble && !reply.isEmpty()) {
                m_assistantStreamingBuffer += reply;
                emit chatAssistantDelta(reply);
            }
            finalizeAiReply(bubble,
                            success,
                            false,
                            success ? QString() : (errMsg.isEmpty() ? QStringLiteral("生成失败") : errMsg));
        }
    }
}

void PetStateChat::deliverChatStdinPayload()
{
    if (!m_process || m_chatStdinPending.isEmpty()) {
        return;
    }
    const QByteArray payload = m_chatStdinPending;
    m_chatStdinPending.clear();

    qint64 totalWritten = 0;
    while (totalWritten < payload.size()) {
        const qint64 remain = static_cast<qint64>(payload.size()) - totalWritten;
        const qint64 w = m_process->write(payload.constData() + totalWritten, remain);
        if (w <= 0) {
            qWarning() << "[聊天AI] stdin 写入失败:" << m_process->errorString();
            m_waitingForAiResponse = false;
            m_process->kill();
            emit chatBusyChanged(false);
            emit showChatReply(QStringLiteral("无法向脚本发送对话数据，请重试。"));
            if (!m_finalizeDone) {
                finalizeAiReply(m_aiAssistantBubbleShown,
                               false,
                               false,
                               QStringLiteral("stdin 写入失败"));
            }
            return;
        }
        totalWritten += w;
    }
    m_process->closeWriteChannel();
}

void PetStateChat::onAiProcessStarted()
{
    deliverChatStdinPayload();

    if (m_waitingForAiResponse && !m_aiAssistantBubbleShown) {
        m_aiAssistantBubbleShown = true;
        emit chatAssistantStarted();
    }
}

void PetStateChat::onProcessStdout()
{
    if (!m_process || m_finalizeDone || m_userCancelled) {
        return;
    }

    m_stdoutLineBuffer.append(m_process->readAllStandardOutput());
    flushStdoutLines();
}

void PetStateChat::onUserInput(const QString& input)
{
    qDebug() << "[聊天] 用户输入:" << input;

    if (input.length() > 200) {
        emit showChatReply(QStringLiteral("输入内容太长啦，我有点记不住~ 请控制在200字以内哦！"));
        return;
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        emit showChatReply(QStringLiteral("我还在想上一个问题呢，稍等一下~"));
        return;
    }

    m_currentInput = input;
    m_assistantStreamingBuffer.clear();
    m_pendingDoneReply.clear();

    PetConfig* config = PetConfig::getInstance();
    const QString projectRootPath = resolveProjectRootPath();

    QString scriptPath = config->getChatScriptPath();
    if (QDir::isRelativePath(scriptPath)) {
        scriptPath = QDir(projectRootPath).absoluteFilePath(scriptPath);
    }
    if (!QFile::exists(scriptPath)) {
        emit showChatReply(QStringLiteral("聊天脚本不存在，请检查 chat_runtime.script_path 配置。"));
        return;
    }

    const QString model = config->getChatModel();

    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();
    int level = m_attr->getLevel();

    QString stateDesc = QStringLiteral("宠物当前状态：饱食度%1，精力%2，心情%3，等级%4。").arg(hunger).arg(energy).arg(mood).arg(level);
    if (hunger < 30) {
        stateDesc += QStringLiteral(" 宠物现在很饥饿。");
    }
    if (energy < 30) {
        stateDesc += QStringLiteral(" 宠物现在很疲劳。");
    }
    if (mood < 30) {
        stateDesc += QStringLiteral(" 宠物现在心情不太好。");
    }

    QJsonObject inputJson;
    inputJson[QStringLiteral("input")] = input;
    inputJson[QStringLiteral("model")] = model;
    inputJson[QStringLiteral("max_tokens")] = 150;
    inputJson[QStringLiteral("temperature")] = 0.7;
    inputJson[QStringLiteral("pet_state")] = stateDesc;
    inputJson[QStringLiteral("ollama_host")] = config->getChatOllamaHost();
    inputJson[QStringLiteral("conversation_summary")] = m_conversationSummary;
    inputJson[QStringLiteral("history")] = buildHistoryPayload();

    const QByteArray payload = QJsonDocument(inputJson).toJson(QJsonDocument::Compact);

    if (!m_process) {
        m_process = new QProcess(this);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
        env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
        env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
        /* OLLAMA_HOST 由用户在系统环境里设置则可覆盖 stdin JSON；不再在此强制写入 */
        m_process->setProcessEnvironment(env);

        QObject::connect(m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                         this, &PetStateChat::onProcessFinished, Qt::UniqueConnection);
        QObject::connect(m_process, &QProcess::errorOccurred,
                         this, &PetStateChat::onProcessError, Qt::UniqueConnection);
        QObject::connect(m_process, &QProcess::readyReadStandardOutput,
                         this, &PetStateChat::onProcessStdout, Qt::UniqueConnection);
        QObject::connect(m_process, &QProcess::started,
                         this, &PetStateChat::onAiProcessStarted, Qt::UniqueConnection);
    }

    m_stdoutLineBuffer.clear();
    m_userCancelled = false;
    m_finalizeDone = false;
    m_waitingForAiResponse = true;
    m_aiAssistantBubbleShown = false;

    QString pythonExecutable = config->getPythonPath().trimmed();
    QStringList args = { scriptPath, QStringLiteral("--stdin-json") };

    if (pythonExecutable.isEmpty()) {
        pythonExecutable = QStringLiteral("python3");
    }
    const QFileInfo pythonInfo(pythonExecutable);
    if (pythonInfo.isAbsolute() && !pythonInfo.exists()) {
        qWarning() << "[聊天AI] 配置的 Python 路径不存在，回退为 python3";
        pythonExecutable = QStringLiteral("python3");
    }

    emit chatBusyChanged(true);

    m_chatStdinPending = payload;

    qDebug() << "[聊天AI] 启动命令:" << pythonExecutable << args;
    m_process->start(pythonExecutable, args);
    /* stdin 在 onAiProcessStarted → deliverChatStdinPayload 中写入，避免 waitForStarted 阻塞 GUI */
}

void PetStateChat::cancelCurrentChat()
{
    if (!m_waitingForAiResponse || !m_process || m_process->state() == QProcess::NotRunning) {
        return;
    }
    m_userCancelled = true;
    m_process->kill();
}

void PetStateChat::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_process) {
        return;
    }

    const QString stderrText = QString::fromUtf8(m_process->readAllStandardError());
    if (!stderrText.isEmpty()) {
        qDebug() << "[聊天AI] 错误输出:" << stderrText;
    }

    m_stdoutLineBuffer.append(m_process->readAllStandardOutput());

    if (m_userCancelled) {
        const bool bubble = m_aiAssistantBubbleShown;
        finalizeAiReply(bubble,
                        /*success=*/false,
                        /*userCancelled=*/true,
                        QString());
        m_stdoutLineBuffer.clear();
        return;
    }

    flushStdoutLines();

    /* 缓冲区内若仍有未换行结尾的半截 JSON，追加换行后再解析一轮 */
    if (!m_finalizeDone && !m_stdoutLineBuffer.trimmed().isEmpty()) {
        QByteArray leftover = m_stdoutLineBuffer.trimmed();
        leftover.append('\n');
        m_stdoutLineBuffer = leftover;
        flushStdoutLines();
    }

    if (!m_finalizeDone) {
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            finalizeAiReply(m_aiAssistantBubbleShown,
                            false,
                            false,
                            m_aiAssistantBubbleShown
                                ? QStringLiteral("未收到完成标记")
                                : QStringLiteral("未收到 AI 响应"));
        } else {
            finalizeAiReply(m_aiAssistantBubbleShown,
                            false,
                            false,
                            QStringLiteral("脚本异常退出 (code=%1)").arg(exitCode));
        }
    }

    m_stdoutLineBuffer.clear();
}

void PetStateChat::onProcessError(QProcess::ProcessError error)
{
    if (!m_process) {
        return;
    }

    qDebug() << "[聊天AI] 进程错误:" << error << m_process->errorString();

    if (!m_waitingForAiResponse || m_finalizeDone) {
        return;
    }

    /* 气泡尚未出现时由 showChatReply 提示；出现后走结束收尾 */
    const bool bubble = m_aiAssistantBubbleShown;
    finalizeAiReply(bubble,
                    false,
                    false,
                    QStringLiteral("聊天服务启动失败：%1").arg(m_process->errorString()));
}

QString PetStateChat::absoluteChatMemoryPath() const
{
    return absoluteChatMemoryPathFromConfig();
}

void PetStateChat::writeEmptyChatMemoryFile()
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("summary")] = QString();
    root[QStringLiteral("messages")] = QJsonArray();

    const QString path = absoluteChatMemoryPathFromConfig();
    const QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "[聊天记忆] 重置：无法写入空存档" << path;
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
}

void PetStateChat::wipeChatTurnsSummaryAndSave()
{
    m_chatMessages.clear();
    m_conversationSummary.clear();
    saveChatMemory();
}

void PetStateChat::loadChatMemory()
{
    m_chatMessages.clear();
    m_conversationSummary.clear();

    const QString path = absoluteChatMemoryPath();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[聊天记忆] JSON 解析失败，忽略存档";
        return;
    }

    const QJsonObject root = doc.object();
    m_conversationSummary = root.value(QStringLiteral("summary")).toString();

    const QJsonArray arr = root.value(QStringLiteral("messages")).toArray();
    const int maxLoad = 400;
    int startIdx = 0;
    if (arr.size() > maxLoad) {
        startIdx = arr.size() - maxLoad;
    }
    for (int i = startIdx; i < arr.size(); ++i) {
        const QJsonValue v = arr.at(i);
        if (!v.isObject()) {
            continue;
        }
        const QJsonObject o = v.toObject();
        const QString role = o.value(QStringLiteral("role")).toString();
        const QString content = o.value(QStringLiteral("content")).toString();
        if (role != QLatin1String("user") && role != QLatin1String("assistant")) {
            continue;
        }
        if (content.isEmpty()) {
            continue;
        }
        m_chatMessages.push_back(ChatMessage{role, content});
    }
}

void PetStateChat::saveChatMemory()
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("summary")] = m_conversationSummary;

    QJsonArray msgs;
    const int maxSave = 500;
    int startIdx = 0;
    if (m_chatMessages.size() > maxSave) {
        startIdx = m_chatMessages.size() - maxSave;
    }
    for (int i = startIdx; i < m_chatMessages.size(); ++i) {
        const ChatMessage& m = m_chatMessages[i];
        QJsonObject o;
        o[QStringLiteral("role")] = m.role;
        o[QStringLiteral("content")] = m.content;
        msgs.append(o);
    }
    root[QStringLiteral("messages")] = msgs;

    const QString path = absoluteChatMemoryPath();
    const QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "[聊天记忆] 无法写入:" << path;
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
}

void PetStateChat::appendSuccessfulExchange(const QString& userText, const QString& assistantText)
{
    m_chatMessages.push_back(ChatMessage{QStringLiteral("user"), userText});
    m_chatMessages.push_back(ChatMessage{QStringLiteral("assistant"), assistantText});
}

QJsonArray PetStateChat::buildHistoryPayload() const
{
    PetConfig* cfg = PetConfig::getInstance();
    const int n = m_chatMessages.size();
    if (n <= 0) {
        return QJsonArray();
    }

    const int maxMsgs = qMin(n, qMax(1, cfg->getChatContextTurns()) * 2);
    int start = n - maxMsgs;
    if (start < 0) {
        start = 0;
    }

    qint64 total = 0;
    const qint64 budget = cfg->getChatMaxContextChars();
    for (int i = n - 1; i >= start; --i) {
        total += static_cast<qint64>(m_chatMessages[i].content.size());
    }
    while (start < n - 1 && total > budget) {
        total -= static_cast<qint64>(m_chatMessages[start].content.size());
        ++start;
    }

    QJsonArray arr;
    for (int i = start; i < n; ++i) {
        QJsonObject o;
        o[QStringLiteral("role")] = m_chatMessages[i].role;
        o[QStringLiteral("content")] = m_chatMessages[i].content;
        arr.append(o);
    }
    return arr;
}

bool PetStateChat::tryCompressOldTurns()
{
    PetConfig* cfg = PetConfig::getInstance();
    if (!cfg->isChatSummaryEnabled()) {
        return false;
    }

    const int n = m_chatMessages.size();
    if (n < 2) {
        return false;
    }

    const int pairs = n / 2;
    const int keep = cfg->getChatSummaryKeepRecentTurns();
    const int trigger = cfg->getChatSummaryCompressAfterTurns();
    if (pairs <= trigger) {
        return false;
    }

    const int removePairs = pairs - keep;
    if (removePairs <= 0) {
        return false;
    }

    const int removeMsgs = removePairs * 2;
    QString chunk;
    chunk.reserve(removeMsgs * 48);
    for (int i = 0; i < removeMsgs; ++i) {
        const ChatMessage& m = m_chatMessages[i];
        chunk += (m.role == QLatin1String("user")) ? QStringLiteral("用户：") : QStringLiteral("助理：");
        chunk += m.content;
        chunk += QLatin1Char('\n');
    }

    const QString merged = runSummarizeMerge(chunk.left(12000));
    if (merged.isEmpty()) {
        qWarning() << "[聊天记忆] 摘要失败，保留原文";
        return false;
    }

    m_conversationSummary = merged;
    m_chatMessages.remove(0, removeMsgs);
    return true;
}

QString PetStateChat::runSummarizeMerge(const QString& dialogueChunk)
{
    PetConfig* config = PetConfig::getInstance();
    const QString projectRootPath = resolveProjectRootPath();

    QString scriptPath = config->getChatScriptPath();
    if (QDir::isRelativePath(scriptPath)) {
        scriptPath = QDir(projectRootPath).absoluteFilePath(scriptPath);
    }
    if (!QFile::exists(scriptPath)) {
        return QString();
    }

    QJsonObject root;
    root[QStringLiteral("mode")] = QStringLiteral("summarize");
    root[QStringLiteral("existing_summary")] = m_conversationSummary;
    root[QStringLiteral("dialogue_chunk")] = dialogueChunk;
    root[QStringLiteral("summary_max_chars")] = config->getChatSummaryMaxChars();
    root[QStringLiteral("model")] = config->getChatModel();
    root[QStringLiteral("ollama_host")] = config->getChatOllamaHost();

    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

    QString pythonExecutable = config->getPythonPath().trimmed();
    if (pythonExecutable.isEmpty()) {
        pythonExecutable = QStringLiteral("python3");
    }
    const QStringList args = {scriptPath, QStringLiteral("--stdin-json")};

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    env.insert(QStringLiteral("PYTHONUTF8"), QStringLiteral("1"));
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    proc.setProcessEnvironment(env);

    proc.start(pythonExecutable, args);

    QElapsedTimer startWait;
    startWait.start();
    while (!proc.waitForStarted(100)) {
        if (proc.state() == QProcess::NotRunning) {
            qWarning() << "[聊天摘要] Python 启动失败";
            return QString();
        }
        if (startWait.elapsed() > 8000) {
            proc.kill();
            qWarning() << "[聊天摘要] Python 启动超时";
            return QString();
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    proc.write(payload);
    proc.closeWriteChannel();

    QElapsedTimer runWait;
    runWait.start();
    while (!proc.waitForFinished(100)) {
        if (runWait.elapsed() > 120000) {
            proc.kill();
            qWarning() << "[聊天摘要] 超时";
            return QString();
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    const QByteArray out = proc.readAllStandardOutput();
    const QList<QByteArray> lines = out.split('\n');
    for (const QByteArray& raw : lines) {
        const QByteArray line = raw.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }
        const QJsonObject obj = doc.object();
        if (obj.value(QStringLiteral("event")).toString() != QStringLiteral("done")) {
            continue;
        }
        if (!obj.value(QStringLiteral("success")).toBool()) {
            return QString();
        }
        return obj.value(QStringLiteral("summary")).toString().trimmed();
    }
    return QString();
}
