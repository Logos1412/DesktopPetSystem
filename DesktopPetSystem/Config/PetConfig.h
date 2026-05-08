#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QHash>
#include <QMap>
#include <QString>

struct FoodData {
    QString id;
    QString name;
    int price;
    int hunger;
    int energy;
    int mood;
    int exp;
};

class PetConfig : public QObject
{
    Q_OBJECT

private:
    PetConfig(QObject* parent = nullptr) : QObject(parent) {}

    // 获取嵌套JSON对象
    QJsonObject getNestedObj(const QJsonObject& root, const QString& key);

    void applyChatRuntimeObject(const QJsonObject& chatRuntimeObj);

    static PetConfig* m_instance;   // 单例实例

    // 默认属性值
    int m_defaultHunger = 50;       // 默认饱食度
    int m_defaultEnergy = 60;       // 默认精力
    int m_defaultMood = 70;         // 默认心情
    int m_defaultLevel = 1;         // 默认等级
    int m_defaultExp = 0;           // 默认经验

    // 待机状态属性变化速率（每分钟，由 JSON 填入；运行时每秒 tick 按比例分摊）
    int m_hungerDecayIdle = 120;
    int m_energyDecayIdle = 120;
    int m_moodDecayIdle = 60;

    // 异常状态属性变化速率（每分钟）
    int m_hungerDecayAbnormal = 60;
    int m_energyDecayAbnormal = 60;
    int m_moodDecayAbnormal = 120;

    // 阈值配置
    int m_abnormalThreshold = 20;   // 进入异常状态的阈值
    int m_autoSleepEnergy = 5;      // 自动睡眠的精力阈值
    int m_recoveryThreshold = 20;   // 恢复正常状态的阈值

    // 睡眠配置
    int m_sleepEnergyRecovery = 480; // 睡眠中精力恢复（每分钟，按秒分摊）
    int m_sleepHungerDecay = 60;     // 睡眠中饱食衰减（每分钟）
    // 玩耍配置
    int m_playMoodIncrease = 600;   // 玩耍时心情增加（每分钟）
    int m_playEnergyDecay = 180;    // 玩耍时精力衰减（每分钟）
    int m_playHungerDecay = 120;    // 玩耍时饱食度衰减（每分钟）
    int m_playExpGain = 120;        // 玩耍时经验增加（每分钟）

    // 工作配置
    int m_workEnergyDecay = 240;    // 工作时精力衰减（每分钟）
    int m_workHungerDecay = 180;    // 工作时饱食度衰减（每分钟）
    int m_workMoodDecay = 120;      // 工作时心情衰减（每分钟）
    int m_workExpGain = 180;        // 工作时经验增加（每分钟）
    int m_workCoinGain = 300;       // 工作时金币增加（每分钟）

    // 学习配置
    int m_studyEnergyDecay = 300;   // 学习时精力衰减（每分钟）
    int m_studyHungerDecay = 120;   // 学习时饱食度衰减（每分钟）
    int m_studyMoodDecay = 60;      // 学习时心情衰减（每分钟）
    int m_studyExpGain = 300;       // 学习时经验增加（每分钟）

    // 聊天：单次 AI 对话成功结束时结算的点数（非「每分钟」速率）
    int m_chatEnergyDecay = 1;
    int m_chatHungerDecay = 1;
    int m_chatMoodIncrease = 2;

    // 经验成长配置
    int m_expBase = 100;            // 基础升级经验
    int m_expGrowth = 20;           // 每级增长经验

    // 聊天运行时配置
    QString m_chatProvider = QStringLiteral("ollama");
    QString m_chatApiBase;
    QString m_chatApiKey;
    QString m_chatApiKeyEnv;
    QString m_chatModel = "llama3.1:8b";
    QString m_chatScriptPath = "resources/scripts/chat_ai.py";
    QString m_pythonPath = "python3";
    QString m_chatOllamaHost = QStringLiteral("http://127.0.0.1:11434");
    /** 对话系统提示中的「宠物人设」正文（chat_runtime.pet_persona）；为空则由脚本内置默认 */
    QString m_chatPetPersona;
    /* 聊天上下文与摘要（chat_runtime） */
    int m_chatContextTurns = 8;
    int m_chatMaxContextChars = 8000;
    /** 传给 Python 的单轮 max_tokens；过小会导致中文说到一半被截断 */
    int m_chatMaxReplyTokens = 1024;
    /** 单次用户输入最大字符（UI 计数与脚本截断一致；换模型可在设置里调整） */
    int m_chatMaxInputChars = 800;
    /** 预留给 system+本轮输入等开销，避免历史塞满导致总长失控 */
    int m_chatContextReserveChars = 2000;
    /** 历史消息估算 token 上限（0=不启用）；配合 chars_per_est_token */
    int m_chatMaxHistoryEstTokens = 0;
    /** 粗略「字/token」换算：中文为主的对话常用 2～4 */
    int m_chatCharsPerEstToken = 3;
    /** 结构化记忆总字符超此值也触发压缩（0=仅按轮数）；防止记忆体积膨胀 */
    int m_chatSummaryCompressAfterMemoryChars = 2800;
    /** system 中默认不要求括号舞台说明，减轻「尾巴」式幻觉 */
    bool m_chatReplyNoStageDirections = true;
    QString m_chatMemoryPath = QStringLiteral("resources/save/chat_memory.json");
    bool m_chatSummaryEnabled = true;
    int m_chatSummaryCompressAfterTurns = 12;
    int m_chatSummaryKeepRecentTurns = 8;
    int m_chatSummaryMaxChars = 400;
    int m_memoryPreferencesMaxChars = 120;
    int m_memoryTasksMaxChars = 120;
    int m_memoryAvoidMaxChars = 80;
    int m_memoryFactsMaxChars = 120;

    // 日志配置
    bool m_verboseStateLogs = false;

    // 移动配置
    int m_idleTimeout = 5000;       // 无操作后开始移动的时间（毫秒）
    qreal m_moveSpeed = 2.0;        // 移动速度（像素/帧）
    int m_moveUpdateInterval = 33;  // 移动更新间隔（毫秒）
    int m_directionKeepRate = 80;   // 保持方向的概率（%）

    // 双击互动
    int m_doubleClickCooldownMs = 10000;     // 双击最小间隔（毫秒）
    int m_doubleClickMoodDeltaIdle = 2;      // 正常待机有效双击·心情
    int m_doubleClickMoodDeltaAbnormal = 1;  // 异常待机有效双击·心情

    // 窗口配置
    int m_windowWidth = 300;        // 窗口宽度
    int m_windowHeight = 300;       // 窗口高度

    // 属性边界
    int m_maxValue = 100;           // 属性最大值
    int m_minValue = 0;             // 属性最小值

    // 食物配置
    QMap<QString, FoodData> m_foods;

    QString m_configFilePath;

    /** resources/animations/ 下相对路径 → 显示缩放系数（约 0.05～4） */
    QHash<QString, double> m_animationScales;

public:
    // 获取单例实例
    static PetConfig* getInstance();

    // 加载配置文件
    bool loadConfig(const QString& path);

    /** 仅从磁盘重新载入 chat_runtime（不发 configReloaded，会发 chatRuntimeSynced）；对话发送前调用以同步人设/API */
    bool reloadChatRuntimeFromFile();

    // 获取默认属性值
    int getDefaultHunger() const { return m_defaultHunger; }
    int getDefaultEnergy() const { return m_defaultEnergy; }
    int getDefaultMood() const { return m_defaultMood; }
    int getDefaultLevel() const { return m_defaultLevel; }
    int getDefaultExp() const { return m_defaultExp; }

    // 待机每分钟变化速率
    int getHungerDecayIdle() const { return m_hungerDecayIdle; }
    int getEnergyDecayIdle() const { return m_energyDecayIdle; }
    int getMoodDecayIdle() const { return m_moodDecayIdle; }

    // 异常待机每分钟变化速率
    int getHungerDecayAbnormal() const { return m_hungerDecayAbnormal; }
    int getEnergyDecayAbnormal() const { return m_energyDecayAbnormal; }
    int getMoodDecayAbnormal() const { return m_moodDecayAbnormal; }

    // 获取阈值配置
    int getAbnormalThreshold() const { return m_abnormalThreshold; }
    int getAutoSleepEnergy() const { return m_autoSleepEnergy; }
    int getRecoveryThreshold() const { return m_recoveryThreshold; }

    // 获取睡眠配置
    int getSleepEnergyRecoveryPerMinute() const { return m_sleepEnergyRecovery; }
    int getSleepHungerDecayPerMinute() const { return m_sleepHungerDecay; }
    // 获取玩耍配置
    int getPlayMoodIncreasePerMinute() const { return m_playMoodIncrease; }
    int getPlayEnergyDecayPerMinute() const { return m_playEnergyDecay; }
    int getPlayHungerDecayPerMinute() const { return m_playHungerDecay; }
    int getPlayExpGainPerMinute() const { return m_playExpGain; }

    // 获取工作配置
    int getWorkEnergyDecayPerMinute() const { return m_workEnergyDecay; }
    int getWorkHungerDecayPerMinute() const { return m_workHungerDecay; }
    int getWorkMoodDecayPerMinute() const { return m_workMoodDecay; }
    int getWorkExpGainPerMinute() const { return m_workExpGain; }
    int getWorkCoinGainPerMinute() const { return m_workCoinGain; }

    // 获取学习配置
    int getStudyEnergyDecayPerMinute() const { return m_studyEnergyDecay; }
    int getStudyHungerDecayPerMinute() const { return m_studyHungerDecay; }
    int getStudyMoodDecayPerMinute() const { return m_studyMoodDecay; }
    int getStudyExpGainPerMinute() const { return m_studyExpGain; }

    // 单次对话成功结束时的点数（非每分钟）
    int getChatEnergyCostPerRound() const { return m_chatEnergyDecay; }
    int getChatHungerCostPerRound() const { return m_chatHungerDecay; }
    int getChatMoodGainPerRound() const { return m_chatMoodIncrease; }

    // 获取经验成长配置
    int getExpBase() const { return m_expBase; }
    int getExpGrowth() const { return m_expGrowth; }

    // 获取聊天运行时配置
    QString getChatProvider() const { return m_chatProvider; }
    QString getChatApiBase() const { return m_chatApiBase; }
    /** 直接写在配置中的 API Key（优先于 api_key_env）；勿提交到公开仓库 */
    QString getChatApiKey() const { return m_chatApiKey; }
    QString getChatApiKeyEnv() const { return m_chatApiKeyEnv; }
    QString getChatModel() const { return m_chatModel; }
    QString getChatScriptPath() const { return m_chatScriptPath; }
    QString getPythonPath() const { return m_pythonPath; }
    QString getChatOllamaHost() const { return m_chatOllamaHost; }
    QString getChatPetPersona() const { return m_chatPetPersona; }
    int getChatContextTurns() const { return m_chatContextTurns; }
    int getChatMaxContextChars() const { return m_chatMaxContextChars; }
    int getChatMaxReplyTokens() const { return m_chatMaxReplyTokens; }
    int getChatMaxInputChars() const { return m_chatMaxInputChars; }
    int getChatContextReserveChars() const { return m_chatContextReserveChars; }
    int getChatMaxHistoryEstTokens() const { return m_chatMaxHistoryEstTokens; }
    int getChatCharsPerEstToken() const { return m_chatCharsPerEstToken; }
    int getChatSummaryCompressAfterMemoryChars() const { return m_chatSummaryCompressAfterMemoryChars; }
    bool isChatReplyNoStageDirections() const { return m_chatReplyNoStageDirections; }
    QString getChatMemoryPath() const { return m_chatMemoryPath; }
    bool isChatSummaryEnabled() const { return m_chatSummaryEnabled; }
    int getChatSummaryCompressAfterTurns() const { return m_chatSummaryCompressAfterTurns; }
    int getChatSummaryKeepRecentTurns() const { return m_chatSummaryKeepRecentTurns; }
    int getChatSummaryMaxChars() const { return m_chatSummaryMaxChars; }
    /** 结构化记忆各槽位最大字符数（写入 Python memory_limits） */
    QJsonObject getStructuredMemoryLimitsJson() const;

    // 获取日志配置
    bool isVerboseStateLogsEnabled() const { return m_verboseStateLogs; }

    // 获取移动配置
    int getIdleTimeout() const { return m_idleTimeout; }
    qreal getMoveSpeed() const { return m_moveSpeed; }
    int getMoveUpdateInterval() const { return m_moveUpdateInterval; }
    int getDirectionKeepRate() const { return m_directionKeepRate; }

    /** 双击最小间隔（毫秒）；0 表示不限制 */
    int getDoubleClickCooldownMs() const { return m_doubleClickCooldownMs; }
    /** 正常待机：每次有效双击增加心情（可为负） */
    int getDoubleClickMoodDeltaIdle() const { return m_doubleClickMoodDeltaIdle; }
    /** 异常待机：每次有效双击增加心情（可为负） */
    int getDoubleClickMoodDeltaAbnormal() const { return m_doubleClickMoodDeltaAbnormal; }

    // 获取窗口配置
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }

    // 获取属性边界
    int getMaxValue() const { return m_maxValue; }
    int getMinValue() const { return m_minValue; }

    // 获取食物配置
    QMap<QString, FoodData> getFoods() const { return m_foods; }
    FoodData getFood(const QString& id) const { return m_foods.value(id); }
    bool hasFood(const QString& id) const { return m_foods.contains(id); }

    /** 最近一次成功 loadConfig 的绝对路径（用于设置界面读写） */
    QString getConfigFilePath() const { return m_configFilePath; }

    /** 规范化动画相对路径键（正斜杠、去前导 /），与 JSON / 表单一致 */
    static QString normalizeAnimationRelativePath(QString path);
    /** 当前 GIF 相对路径的显示缩放；未配置时为 1.0 */
    double animationScaleForRelativePath(const QString& relativePath) const;

signals:
    /** loadConfig 成功解析并写入内存后发出（含首次启动与设置里保存后的热更新） */
    void configReloaded();
    /** 仅 chat_runtime 从磁盘同步后发出（对话发送前 reloadChatRuntimeFromFile） */
    void chatRuntimeSynced();
};
