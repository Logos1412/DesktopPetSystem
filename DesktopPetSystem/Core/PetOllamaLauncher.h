#pragma once
#pragma execution_character_set("utf-8")

#include <QString>

/** 按配置在桌宠启动或设置保存后拉起本地 Ollama（ollama serve）。 */
namespace PetOllamaLauncher {

bool queryTagsOk(const QString& hostBase, QString* detailOut = nullptr);

/** provider 为 ollama 时：若未就绪则启动并等待。默认受 auto_start_ollama 约束；设置保存后可 force。 */
bool ensureServiceRunning(bool forceRegardlessOfAutoStart = false);

void shutdownIfStartedByApp();

} // namespace PetOllamaLauncher
