#pragma execution_character_set("utf-8")

#include "PetGptsovitsLauncher.h"
#include "Config/PetConfig.h"
#include "Config/PetVirtualPath.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace {

QProcess* g_apiProcess = nullptr;
bool g_startedByApp = false;

bool isTtsApiReachable(const QString& baseUrl, int timeoutMs)
{
    const QUrl base(baseUrl.trimmed());
    if (!base.isValid() || base.host().isEmpty()) {
        return false;
    }

    QUrl url(base);
    url.setPath(QStringLiteral("/tts"));

    QNetworkAccessManager nam;
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply* reply = nam.post(req, QByteArray("{}"));
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    const bool timedOut = !reply->isFinished();
    const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    if (timedOut) {
        return false;
    }
    /* api_v2：参数不全时 400；成功合成 200；WebUI 无此路由 404 */
    return code == 200 || code == 400;
}

QString resolveInstallDir()
{
    PetConfig* cfg = PetConfig::getInstance();
    const QString raw = cfg->getTtsGptsovitsInstallDir().trimmed();
    if (raw.isEmpty()) {
        return {};
    }
    if (QDir(raw).isAbsolute()) {
        return QDir(raw).absolutePath();
    }
    const QString root = PetVirtualPath::findProjectRootFromExe();
    return PetVirtualPath::resolveToAbsolute(raw, root);
}

QString resolvePythonExecutable(const QString& installDir)
{
    PetConfig* cfg = PetConfig::getInstance();
    const QString configured = cfg->getTtsGptsovitsApiPython().trimmed();
    if (!configured.isEmpty()) {
        if (QDir(configured).isAbsolute()) {
            return configured;
        }
        return QDir(installDir).absoluteFilePath(configured);
    }
    const QString bundled = QDir(installDir).absoluteFilePath(QStringLiteral("runtime/python.exe"));
    if (QFileInfo::exists(bundled)) {
        return bundled;
    }
    return QStringLiteral("python");
}

bool runStartScript(const QString& scriptAbs)
{
    const QFileInfo fi(scriptAbs);
    if (!fi.exists() || !fi.isFile()) {
        qWarning() << "[GPT-SoVITS] 启动脚本不存在:" << scriptAbs;
        return false;
    }

    QStringList args;
    QString program;
#ifdef Q_OS_WIN
    program = QStringLiteral("cmd.exe");
    args << QStringLiteral("/C") << QDir::toNativeSeparators(scriptAbs);
#else
    program = QStringLiteral("/bin/sh");
    args << scriptAbs;
#endif

    if (!g_apiProcess) {
        g_apiProcess = new QProcess(qApp);
    }
    g_apiProcess->setProgram(program);
    g_apiProcess->setArguments(args);
    g_apiProcess->setWorkingDirectory(fi.absolutePath());
    g_apiProcess->start();
    if (!g_apiProcess->waitForStarted(15000)) {
        qWarning() << "[GPT-SoVITS] 启动脚本未能运行:" << g_apiProcess->errorString();
        return false;
    }
    g_startedByApp = true;
    qDebug() << "[GPT-SoVITS] 已通过脚本启动 API:" << scriptAbs;
    return true;
}

bool startApiProcess()
{
    PetConfig* cfg = PetConfig::getInstance();

    const QString root = PetVirtualPath::findProjectRootFromExe();
    const QString scriptVirtual = cfg->getTtsGptsovitsApiStartScript().trimmed();
    if (!scriptVirtual.isEmpty()) {
        const QString scriptAbs = PetVirtualPath::resolveToAbsolute(
            PetVirtualPath::normalizeConfigurablePath(scriptVirtual), root);
        if (runStartScript(scriptAbs)) {
            return true;
        }
    }

    const QString installDir = resolveInstallDir();
    if (installDir.isEmpty()) {
        qWarning() << "[GPT-SoVITS] 未配置 install_dir 或 api_start_script，无法自动启动 API。"
                   << "请在 pet_config.json 的 tts_gptsovits 中填写 GPT-SoVITS 安装目录。";
        return false;
    }

    const QString apiPy = QDir(installDir).absoluteFilePath(QStringLiteral("api_v2.py"));
    if (!QFileInfo::exists(apiPy)) {
        qWarning() << "[GPT-SoVITS] 未找到 api_v2.py:" << apiPy;
        return false;
    }

    const QString pythonExe = resolvePythonExecutable(installDir);
    const QUrl baseUrl(cfg->getTtsGptsovitsBaseUrl());
    QString host = cfg->getTtsGptsovitsBindHost().trimmed();
    if (host.isEmpty()) {
        host = baseUrl.host();
    }
    if (host.isEmpty()) {
        host = QStringLiteral("127.0.0.1");
    }
    const int port = baseUrl.port(9880);

    QString configRel = cfg->getTtsGptsovitsApiConfig().trimmed();
    if (configRel.isEmpty()) {
        configRel = QStringLiteral("GPT_SoVITS/configs/tts_infer.yaml");
    }
    const QString configPath = QDir(installDir).absoluteFilePath(configRel);

    if (!g_apiProcess) {
        g_apiProcess = new QProcess(qApp);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString gptPath = QDir(installDir).absoluteFilePath(QStringLiteral("GPT_SoVITS"));
    const QString oldPyPath = env.value(QStringLiteral("PYTHONPATH"));
    env.insert(QStringLiteral("PYTHONPATH"),
               oldPyPath.isEmpty() ? gptPath : gptPath + QLatin1Char(';') + oldPyPath);
    g_apiProcess->setProcessEnvironment(env);
    g_apiProcess->setWorkingDirectory(installDir);
    g_apiProcess->setProgram(pythonExe);
    g_apiProcess->setArguments(
        {apiPy,
         QStringLiteral("-a"),
         host,
         QStringLiteral("-p"),
         QString::number(port),
         QStringLiteral("-c"),
         configPath});

    qDebug() << "[GPT-SoVITS] 启动 API:" << pythonExe << g_apiProcess->arguments();
    g_apiProcess->start();
    if (!g_apiProcess->waitForStarted(20000)) {
        qWarning() << "[GPT-SoVITS] API 进程启动失败:" << g_apiProcess->errorString();
        return false;
    }

    g_startedByApp = true;
    return true;
}

} // namespace

namespace PetGptsovitsLauncher {

bool ensureApiRunning(bool forceRegardlessOfAutoStart)
{
    PetConfig* cfg = PetConfig::getInstance();
    if (!cfg->isTtsGptsovitsEnabled()) {
        return true;
    }
    if (!forceRegardlessOfAutoStart && !cfg->isTtsGptsovitsAutoStartApi()) {
        return true;
    }

    const QString baseUrl = cfg->getTtsGptsovitsBaseUrl();
    if (isTtsApiReachable(baseUrl, 2500)) {
        qDebug() << "[GPT-SoVITS] API 已在运行:" << baseUrl;
        return true;
    }

    qDebug() << "[GPT-SoVITS] API 未就绪，尝试启动…";
    bool started = false;
    QMetaObject::invokeMethod(
        qApp,
        [&]() { started = startApiProcess(); },
        Qt::BlockingQueuedConnection);
    if (!started) {
        return false;
    }

    const int waitMs = qMax(5000, cfg->getTtsGptsovitsStartupWaitMs());
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < waitMs) {
        if (isTtsApiReachable(baseUrl, 3000)) {
            qDebug() << "[GPT-SoVITS] API 已就绪，耗时" << timer.elapsed() << "ms";
            return true;
        }
        QThread::msleep(500);
    }

    qWarning() << "[GPT-SoVITS] 等待 API 超时（" << waitMs << "ms），请检查 install_dir 与端口"
               << cfg->getTtsGptsovitsBaseUrl();
    return false;
}

void shutdownIfStartedByApp()
{
    if (!g_startedByApp || !g_apiProcess) {
        return;
    }
    QProcess* proc = g_apiProcess;
    g_apiProcess = nullptr;
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
    qDebug() << "[GPT-SoVITS] 已发送结束信号（非阻塞，不卡住退出）";
}

} // namespace PetGptsovitsLauncher
