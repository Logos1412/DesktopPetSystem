#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
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
    int m_sleepTransitionDelay = 2; // 入睡→睡眠中动画切换延迟（秒，仅展示）

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
    QString m_chatModel = "llama3.1:8b";
    QString m_chatScriptPath = "resources/scripts/chat_ai.py";
    QString m_pythonPath = "python3";
    QString m_chatOllamaHost = QStringLiteral("http://127.0.0.1:11434");
    /* 聊天上下文与摘要（chat_runtime） */
    int m_chatContextTurns = 8;
    int m_chatMaxContextChars = 8000;
    QString m_chatMemoryPath = QStringLiteral("resources/save/chat_memory.json");
    bool m_chatSummaryEnabled = true;
    int m_chatSummaryCompressAfterTurns = 12;
    int m_chatSummaryKeepRecentTurns = 8;
    int m_chatSummaryMaxChars = 400;

    // 日志配置
    bool m_verboseStateLogs = false;

    // 移动配置
    int m_idleTimeout = 5000;       // 无操作后开始移动的时间（毫秒）
    qreal m_moveSpeed = 2.0;        // 移动速度（像素/帧）
    int m_moveUpdateInterval = 33;  // 移动更新间隔（毫秒）
    int m_directionKeepRate = 80;   // 保持方向的概率（%）

    // 动画配置
    int m_specialAnimationDuration = 2000;  // 特殊动画播放时长（毫秒）

    // 窗口配置
    int m_windowWidth = 200;        // 窗口宽度
    int m_windowHeight = 200;       // 窗口高度

    // 属性边界
    int m_maxValue = 100;           // 属性最大值
    int m_minValue = 0;             // 属性最小值

    // 食物配置
    QMap<QString, FoodData> m_foods;

public:
    // 获取单例实例
    static PetConfig* getInstance();

    // 加载配置文件
    bool loadConfig(const QString& path);

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
    int getSleepTransitionDelay() const { return m_sleepTransitionDelay; }

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
    QString getChatModel() const { return m_chatModel; }
    QString getChatScriptPath() const { return m_chatScriptPath; }
    QString getPythonPath() const { return m_pythonPath; }
    QString getChatOllamaHost() const { return m_chatOllamaHost; }
    int getChatContextTurns() const { return m_chatContextTurns; }
    int getChatMaxContextChars() const { return m_chatMaxContextChars; }
    QString getChatMemoryPath() const { return m_chatMemoryPath; }
    bool isChatSummaryEnabled() const { return m_chatSummaryEnabled; }
    int getChatSummaryCompressAfterTurns() const { return m_chatSummaryCompressAfterTurns; }
    int getChatSummaryKeepRecentTurns() const { return m_chatSummaryKeepRecentTurns; }
    int getChatSummaryMaxChars() const { return m_chatSummaryMaxChars; }

    // 获取日志配置
    bool isVerboseStateLogsEnabled() const { return m_verboseStateLogs; }

    // 获取移动配置
    int getIdleTimeout() const { return m_idleTimeout; }
    qreal getMoveSpeed() const { return m_moveSpeed; }
    int getMoveUpdateInterval() const { return m_moveUpdateInterval; }
    int getDirectionKeepRate() const { return m_directionKeepRate; }

    // 获取动画配置
    int getSpecialAnimationDuration() const { return m_specialAnimationDuration; }

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
};
