#pragma execution_character_set("utf-8")

#include "PetInferenceServices.h"
#include "PetGptsovitsLauncher.h"
#include "PetOllamaLauncher.h"
#include "Config/PetConfig.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QtConcurrent/QtConcurrent>

namespace {

QMutex g_scheduleMutex;
bool g_backgroundBusy = false;

void runEnsureJob(bool forceRegardlessOfAutoStart)
{
    PetConfig* cfg = PetConfig::getInstance();
    qDebug() << "[推理服务] 后台启动任务开始"
             << (forceRegardlessOfAutoStart ? QStringLiteral("(设置保存)") : QStringLiteral("(程序启动)"));

    if (cfg->getChatProvider() == QStringLiteral("ollama")) {
        PetOllamaLauncher::ensureServiceRunning(forceRegardlessOfAutoStart);
    }
    if (cfg->isTtsGptsovitsEnabled()) {
        PetGptsovitsLauncher::ensureApiRunning(forceRegardlessOfAutoStart);
    }

    qDebug() << "[推理服务] 后台启动任务结束";
}

void scheduleEnsure(bool forceRegardlessOfAutoStart)
{
    QMutexLocker lock(&g_scheduleMutex);
    if (g_backgroundBusy) {
        qDebug() << "[推理服务] 已有后台启动任务进行中，跳过本次调度";
        return;
    }
    g_backgroundBusy = true;

    (void)QtConcurrent::run([forceRegardlessOfAutoStart]() {
        runEnsureJob(forceRegardlessOfAutoStart);
        QMutexLocker doneLock(&g_scheduleMutex);
        g_backgroundBusy = false;
    });
}

} // namespace

void ensureConfiguredInferenceServicesAtStartup()
{
    scheduleEnsure(false);
}

void ensureConfiguredInferenceServicesAfterSettingsSave()
{
    scheduleEnsure(true);
}

void shutdownInferenceServicesOnApplicationQuit()
{
    PetOllamaLauncher::shutdownIfStartedByApp();
    PetGptsovitsLauncher::shutdownIfStartedByApp();
}
