#pragma once
#pragma execution_character_set("utf-8")

/** 在后台线程拉起 Ollama / GPT-SoVITS，不阻塞主界面与设置保存。 */
void ensureConfiguredInferenceServicesAtStartup();
void ensureConfiguredInferenceServicesAfterSettingsSave();
/** 应用退出时结束本程序拉起的推理/TTS 进程（不阻塞 UI） */
void shutdownInferenceServicesOnApplicationQuit();
