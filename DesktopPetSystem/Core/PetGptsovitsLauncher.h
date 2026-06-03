#pragma once
#pragma execution_character_set("utf-8")

/** 按配置在桌宠启动时拉起 GPT-SoVITS api_v2，退出时结束本程序拉起的进程。 */
namespace PetGptsovitsLauncher {

/** enabled 时：若 API 未就绪则尝试启动。默认受 auto_start_api 约束；设置保存后可 force。 */
bool ensureApiRunning(bool forceRegardlessOfAutoStart = false);

/** 应用退出时调用：仅结束由本模块 start 的 API 进程。 */
void shutdownIfStartedByApp();

} // namespace PetGptsovitsLauncher
