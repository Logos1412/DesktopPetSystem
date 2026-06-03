#pragma execution_character_set("utf-8")

#include "PetOllamaLauncher.h"
#include "Config/PetConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace {

QProcess* g_ollamaProcess = nullptr;
bool g_startedByApp = false;

} // namespace

namespace PetOllamaLauncher {

bool queryTagsOk(const QString& hostBase, QString* detailOut)
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
    QObject::connect(reply, &QNetworkReply::finished, &loop, [&]() {
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

namespace {

QString resolveOllamaExecutable()
{
    PetConfig* cfg = PetConfig::getInstance();
    const QString configured = cfg->getChatOllamaExecutable().trimmed();
    if (!configured.isEmpty()) {
        if (QFileInfo::exists(configured)) {
            return configured;
        }
        return configured;
    }

    const QString localAppData = QProcessEnvironment::systemEnvironment().value(QStringLiteral("LOCALAPPDATA"));
    if (!localAppData.isEmpty()) {
        const QString bundled =
            QDir(localAppData).absoluteFilePath(QStringLiteral("Programs/Ollama/ollama.exe"));
        if (QFileInfo::exists(bundled)) {
            return bundled;
        }
    }

    return QStringLiteral("ollama");
}

bool startOllamaServeProcess()
{
    const QString exe = resolveOllamaExecutable();
    if (exe != QStringLiteral("ollama") && !QFileInfo::exists(exe)) {
        qWarning() << "[Ollama] 可执行文件不存在:" << exe;
        return false;
    }

    if (!g_ollamaProcess) {
        g_ollamaProcess = new QProcess(qApp);
    }

    g_ollamaProcess->setProgram(exe);
    g_ollamaProcess->setArguments({QStringLiteral("serve")});
    g_ollamaProcess->setProcessChannelMode(QProcess::MergedChannels);

    qDebug() << "[Ollama] 启动:" << exe << "serve";
    g_ollamaProcess->start();
    if (!g_ollamaProcess->waitForStarted(15000)) {
        qWarning() << "[Ollama] 启动失败:" << g_ollamaProcess->errorString();
        return false;
    }

    g_startedByApp = true;
    return true;
}

} // namespace

bool ensureServiceRunning(bool forceRegardlessOfAutoStart)
{
    PetConfig* cfg = PetConfig::getInstance();
    if (cfg->getChatProvider() != QStringLiteral("ollama")) {
        return true;
    }
    if (!forceRegardlessOfAutoStart && !cfg->isChatAutoStartOllama()) {
        return true;
    }

    const QString host = cfg->getChatOllamaHost();
    if (queryTagsOk(host, nullptr)) {
        qDebug() << "[Ollama] 服务已在运行:" << host;
        return true;
    }

    qDebug() << "[Ollama] 服务未就绪，尝试启动 ollama serve…";
    bool started = false;
    QMetaObject::invokeMethod(
        qApp,
        [&]() { started = startOllamaServeProcess(); },
        Qt::BlockingQueuedConnection);
    if (!started) {
        return false;
    }

    const int waitMs = qMax(3000, cfg->getChatOllamaStartupWaitMs());
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < waitMs) {
        if (queryTagsOk(host, nullptr)) {
            qDebug() << "[Ollama] 服务已就绪，耗时" << timer.elapsed() << "ms";
            return true;
        }
        QThread::msleep(400);
    }

    qWarning() << "[Ollama] 等待服务超时（" << waitMs << "ms）:" << host;
    return false;
}

void shutdownIfStartedByApp()
{
    if (!g_startedByApp || !g_ollamaProcess) {
        return;
    }
    QProcess* proc = g_ollamaProcess;
    g_ollamaProcess = nullptr;
    g_startedByApp = false;
    if (proc->state() != QProcess::NotRunning) {
        proc->terminate();
    }
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), proc, [proc]() {
        if (proc->state() != QProcess::NotRunning) {
            proc->kill();
        }
        proc->deleteLater();
    });
    QTimer::singleShot(2000, proc, [proc]() {
        if (proc->state() == QProcess::NotRunning) {
            return;
        }
        proc->kill();
    });
    qDebug() << "[Ollama] 已发送结束信号（非阻塞，不卡住退出）";
}

} // namespace PetOllamaLauncher
