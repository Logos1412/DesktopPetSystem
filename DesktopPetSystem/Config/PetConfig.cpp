#pragma execution_character_set("utf-8")

#include "PetConfig.h"
#include <QFileInfo>

PetConfig* PetConfig::m_instance = nullptr;

// 获取单例实例
PetConfig* PetConfig::getInstance() {
    if (m_instance == nullptr) {
        m_instance = new PetConfig();
    }
    return m_instance;
}

// 获取嵌套JSON对象
QJsonObject PetConfig::getNestedObj(const QJsonObject& root, const QString& key) {
    if (!root.contains(key)) {
        qWarning() << "[配置警告] 缺少" << key << "节点，使用默认值";
        return QJsonObject();
    }

    QJsonValue value = root[key];
    if (!value.isObject()) {
        qWarning() << "[配置警告]" << key << "节点不是JSON对象，使用默认值";
        return QJsonObject();
    }

    return value.toObject();
}

// 加载配置文件
bool PetConfig::loadConfig(const QString& path) {
    qDebug() << "[配置加载] 尝试读取:" << path;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[配置错误] 文件打开失败:" << file.errorString();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    qDebug() << "[配置加载] 读取内容:" << jsonData;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (doc.isNull() || !doc.isObject()) {
        qCritical() << "[配置错误] JSON解析失败:" << parseError.errorString()
            << "位置:" << parseError.offset;
        return false;
    }

    QJsonObject rootObj = doc.object();

    m_configFilePath = QFileInfo(path).absoluteFilePath();

    // 读取属性配置
    QJsonObject attrObj = getNestedObj(rootObj, "attributes");
    m_defaultHunger = attrObj.value("default_hunger").toInt(50);
    m_defaultEnergy = attrObj.value("default_energy").toInt(60);
    m_defaultMood = attrObj.value("default_mood").toInt(70);
    m_defaultLevel = attrObj.value("default_level").toInt(1);
    m_defaultExp = attrObj.value("default_exp").toInt(0);
    /* 待机/异常：单位为「每分钟」属性变化点数 */
    m_hungerDecayIdle = attrObj.value("hunger_decay_idle").toInt(120);
    m_energyDecayIdle = attrObj.value("energy_decay_idle").toInt(120);
    m_moodDecayIdle = attrObj.value("mood_decay_idle").toInt(60);
    m_hungerDecayAbnormal = attrObj.value("hunger_decay_abnormal").toInt(60);
    m_energyDecayAbnormal = attrObj.value("energy_decay_abnormal").toInt(60);
    m_moodDecayAbnormal = attrObj.value("mood_decay_abnormal").toInt(120);

    // 读取阈值配置
    QJsonObject thresholdObj = getNestedObj(rootObj, "thresholds");
    m_abnormalThreshold = thresholdObj.value("abnormal").toInt(20);
    m_autoSleepEnergy = thresholdObj.value("auto_sleep_energy").toInt(5);
    m_recoveryThreshold = thresholdObj.value("recovery").toInt(20);

    // 读取睡眠配置
    QJsonObject sleepObj = getNestedObj(rootObj, "sleep");
    /* 睡眠中段：每分钟点数；旧版「每秒」字段无 _per_minute 时乘以 60 */
    if (sleepObj.contains(QStringLiteral("energy_recovery_per_minute"))) {
        m_sleepEnergyRecovery = sleepObj.value("energy_recovery_per_minute").toInt(480);
    } else {
        m_sleepEnergyRecovery = sleepObj.value("energy_recovery").toInt(8) * 60;
    }
    if (sleepObj.contains(QStringLiteral("hunger_decay_per_minute"))) {
        m_sleepHungerDecay = sleepObj.value("hunger_decay_per_minute").toInt(60);
    } else {
        m_sleepHungerDecay = sleepObj.value("hunger_decay").toInt(1) * 60;
    }
    // 读取玩耍配置
    QJsonObject playObj = getNestedObj(rootObj, "play");
    if (playObj.contains(QStringLiteral("mood_increase_per_minute"))) {
        m_playMoodIncrease = playObj.value("mood_increase_per_minute").toInt(600);
    } else {
        m_playMoodIncrease = playObj.value("mood_increase").toInt(10) * 60;
    }
    if (playObj.contains(QStringLiteral("energy_decay_per_minute"))) {
        m_playEnergyDecay = playObj.value("energy_decay_per_minute").toInt(180);
    } else {
        m_playEnergyDecay = playObj.value("energy_decay").toInt(3) * 60;
    }
    if (playObj.contains(QStringLiteral("hunger_decay_per_minute"))) {
        m_playHungerDecay = playObj.value("hunger_decay_per_minute").toInt(120);
    } else {
        m_playHungerDecay = playObj.value("hunger_decay").toInt(2) * 60;
    }
    if (playObj.contains(QStringLiteral("exp_gain_per_minute"))) {
        m_playExpGain = playObj.value("exp_gain_per_minute").toInt(120);
    } else {
        m_playExpGain = playObj.value("exp_gain").toInt(2) * 60;
    }

    // 读取工作配置
    QJsonObject workObj = getNestedObj(rootObj, "work");
    if (workObj.contains(QStringLiteral("energy_decay_per_minute"))) {
        m_workEnergyDecay = workObj.value("energy_decay_per_minute").toInt(240);
    } else {
        m_workEnergyDecay = workObj.value("energy_decay").toInt(4) * 60;
    }
    if (workObj.contains(QStringLiteral("hunger_decay_per_minute"))) {
        m_workHungerDecay = workObj.value("hunger_decay_per_minute").toInt(180);
    } else {
        m_workHungerDecay = workObj.value("hunger_decay").toInt(3) * 60;
    }
    if (workObj.contains(QStringLiteral("mood_decay_per_minute"))) {
        m_workMoodDecay = workObj.value("mood_decay_per_minute").toInt(120);
    } else {
        m_workMoodDecay = workObj.value("mood_decay").toInt(2) * 60;
    }
    if (workObj.contains(QStringLiteral("exp_gain_per_minute"))) {
        m_workExpGain = workObj.value("exp_gain_per_minute").toInt(180);
    } else {
        m_workExpGain = workObj.value("exp_gain").toInt(3) * 60;
    }
    if (workObj.contains(QStringLiteral("coin_gain_per_minute"))) {
        m_workCoinGain = workObj.value("coin_gain_per_minute").toInt(300);
    } else {
        m_workCoinGain = workObj.value("coin_gain").toInt(5) * 60;
    }

    // 读取学习配置
    QJsonObject studyObj = getNestedObj(rootObj, "study");
    if (studyObj.contains(QStringLiteral("energy_decay_per_minute"))) {
        m_studyEnergyDecay = studyObj.value("energy_decay_per_minute").toInt(300);
    } else {
        m_studyEnergyDecay = studyObj.value("energy_decay").toInt(5) * 60;
    }
    if (studyObj.contains(QStringLiteral("hunger_decay_per_minute"))) {
        m_studyHungerDecay = studyObj.value("hunger_decay_per_minute").toInt(120);
    } else {
        m_studyHungerDecay = studyObj.value("hunger_decay").toInt(2) * 60;
    }
    if (studyObj.contains(QStringLiteral("mood_decay_per_minute"))) {
        m_studyMoodDecay = studyObj.value("mood_decay_per_minute").toInt(60);
    } else {
        m_studyMoodDecay = studyObj.value("mood_decay").toInt(1) * 60;
    }
    if (studyObj.contains(QStringLiteral("exp_gain_per_minute"))) {
        m_studyExpGain = studyObj.value("exp_gain_per_minute").toInt(300);
    } else {
        m_studyExpGain = studyObj.value("exp_gain").toInt(5) * 60;
    }

    // 读取聊天配置
    QJsonObject chatObj = getNestedObj(rootObj, "chat");
    /* 聊天：仍为「单次 AI 对话成功结算」点数，非每分钟 */
    m_chatEnergyDecay = chatObj.value("energy_cost_per_round").toInt(chatObj.value("energy_decay").toInt(1));
    m_chatHungerDecay = chatObj.value("hunger_cost_per_round").toInt(chatObj.value("hunger_decay").toInt(1));
    m_chatMoodIncrease = chatObj.value("mood_gain_per_round").toInt(chatObj.value("mood_increase").toInt(2));

    // 读取成长配置
    QJsonObject progressionObj = getNestedObj(rootObj, "progression");
    m_expBase = progressionObj.value("exp_base").toInt(100);
    m_expGrowth = progressionObj.value("exp_growth").toInt(20);

    // 读取聊天运行时配置
    applyChatRuntimeObject(getNestedObj(rootObj, "chat_runtime"));

    // 读取日志配置
    QJsonObject loggingObj = getNestedObj(rootObj, "logging");
    m_verboseStateLogs = loggingObj.value("verbose_state_logs").toBool(false);

    // 读取移动配置
    QJsonObject moveObj = getNestedObj(rootObj, "movement");
    m_idleTimeout = moveObj.value("idle_timeout").toInt(5000);
    m_moveSpeed = moveObj.value("move_speed").toDouble(2.0);
    m_moveUpdateInterval = moveObj.value("update_interval").toInt(33);
    m_directionKeepRate = moveObj.value("direction_keep_rate").toInt(80);

    QJsonObject interactionObj = getNestedObj(rootObj, "interaction");
    m_doubleClickCooldownMs = interactionObj.value("double_click_cooldown_ms").toInt(10000);
    if (m_doubleClickCooldownMs < 0) {
        m_doubleClickCooldownMs = 0;
    }
    m_doubleClickMoodDeltaIdle = interactionObj.value(QStringLiteral("double_click_mood_delta_idle")).toInt(2);
    m_doubleClickMoodDeltaAbnormal = interactionObj.value(QStringLiteral("double_click_mood_delta_abnormal")).toInt(1);

    // 读取窗口配置
    QJsonObject windowObj = getNestedObj(rootObj, "window");
    m_windowWidth = windowObj.value("width").toInt(300);
    m_windowHeight = windowObj.value("height").toInt(300);

    // 读取属性边界
    QJsonObject limitsObj = getNestedObj(rootObj, "limits");
    m_maxValue = limitsObj.value("max_value").toInt(100);
    m_minValue = limitsObj.value("min_value").toInt(0);

    // 读取食物配置
    m_foods.clear();
    QJsonObject foodsObj = getNestedObj(rootObj, "foods");
    for (auto it = foodsObj.begin(); it != foodsObj.end(); ++it) {
        QJsonObject foodObj = it.value().toObject();
        FoodData food;
        food.id = it.key();
        food.name = foodObj.value("name").toString();
        food.price = foodObj.value("price").toInt(10);
        food.hunger = foodObj.value("hunger").toInt(20);
        food.energy = foodObj.value("energy").toInt(0);
        food.mood = foodObj.value("mood").toInt(5);
        food.exp = foodObj.value("exp").toInt(1);
        m_foods[it.key()] = food;
    }

    m_animationScales.clear();
    if (rootObj.contains(QStringLiteral("animation_scales"))
        && rootObj.value(QStringLiteral("animation_scales")).isObject()) {
        const QJsonObject animScObj = rootObj.value(QStringLiteral("animation_scales")).toObject();
        for (auto it = animScObj.begin(); it != animScObj.end(); ++it) {
            double v = it.value().toDouble(1.0);
            v = qBound(0.05, v, 4.0);
            m_animationScales.insert(normalizeAnimationRelativePath(it.key()), v);
        }
    }

    qDebug() << "[配置加载成功] =========";
    qDebug() << "默认属性: 饱食=" << m_defaultHunger << "精力=" << m_defaultEnergy << "心情=" << m_defaultMood;
    qDebug() << "阈值: 异常=" << m_abnormalThreshold << "自动睡眠=" << m_autoSleepEnergy << "恢复=" << m_recoveryThreshold;
    qDebug() << "睡眠: 精力恢复(每分钟)=" << m_sleepEnergyRecovery << "饱腹衰减(每分钟)=" << m_sleepHungerDecay;
    qDebug() << "移动: 超时=" << m_idleTimeout << "速度=" << m_moveSpeed;
    qDebug() << "成长: base=" << m_expBase << "growth=" << m_expGrowth;
    qDebug() << "========================";

    emit configReloaded();
    return true;
}

void PetConfig::applyChatRuntimeObject(const QJsonObject& chatRuntimeObj)
{
    m_chatProvider = chatRuntimeObj.value("provider").toString("ollama").trimmed().toLower();
    m_chatApiBase = chatRuntimeObj.value("api_base").toString("").trimmed();
    m_chatApiKey = chatRuntimeObj.value("api_key").toString("").trimmed();
    m_chatApiKeyEnv = chatRuntimeObj.value("api_key_env").toString("").trimmed();
    m_chatModel = chatRuntimeObj.value("model").toString("llama3.1:8b");
    m_chatScriptPath = chatRuntimeObj.value("script_path").toString("resources/scripts/chat_ai.py");
    m_pythonPath = chatRuntimeObj.value("python_path").toString("python3");
    m_chatOllamaHost = chatRuntimeObj.value("ollama_host").toString("http://127.0.0.1:11434");
    m_chatPetPersona = chatRuntimeObj.value("pet_persona").toString();
    m_chatContextTurns = chatRuntimeObj.value("context_turns").toInt(8);
    m_chatMaxContextChars = chatRuntimeObj.value("max_context_chars").toInt(8000);
    m_chatMaxReplyTokens = chatRuntimeObj.value("max_reply_tokens").toInt(1024);
    if (m_chatMaxReplyTokens < 64) {
        m_chatMaxReplyTokens = 64;
    }
    if (m_chatMaxReplyTokens > 8192) {
        m_chatMaxReplyTokens = 8192;
    }
    m_chatMaxInputChars = chatRuntimeObj.value("max_input_chars").toInt(800);
    if (m_chatMaxInputChars < 50) {
        m_chatMaxInputChars = 50;
    }
    if (m_chatMaxInputChars > 32000) {
        m_chatMaxInputChars = 32000;
    }
    m_chatContextReserveChars = chatRuntimeObj.value("context_reserve_chars").toInt(2000);
    if (m_chatContextReserveChars < 200) {
        m_chatContextReserveChars = 200;
    }
    if (m_chatContextReserveChars > 50000) {
        m_chatContextReserveChars = 50000;
    }
    m_chatMaxHistoryEstTokens = chatRuntimeObj.value("max_history_estimated_tokens").toInt(0);
    if (m_chatMaxHistoryEstTokens < 0) {
        m_chatMaxHistoryEstTokens = 0;
    }
    if (m_chatMaxHistoryEstTokens > 500000) {
        m_chatMaxHistoryEstTokens = 500000;
    }
    m_chatCharsPerEstToken = chatRuntimeObj.value("context_chars_per_est_token").toInt(3);
    if (m_chatCharsPerEstToken < 1) {
        m_chatCharsPerEstToken = 1;
    }
    if (m_chatCharsPerEstToken > 16) {
        m_chatCharsPerEstToken = 16;
    }
    m_chatSummaryCompressAfterMemoryChars = chatRuntimeObj.value("summary_compress_after_memory_chars").toInt(2800);
    if (m_chatSummaryCompressAfterMemoryChars < 0) {
        m_chatSummaryCompressAfterMemoryChars = 0;
    }
    if (m_chatSummaryCompressAfterMemoryChars > 500000) {
        m_chatSummaryCompressAfterMemoryChars = 500000;
    }
    m_chatReplyNoStageDirections = chatRuntimeObj.value("reply_no_stage_directions").toBool(true);
    m_chatMemoryPath = chatRuntimeObj.value("memory_path").toString("resources/save/chat_memory.json");
    m_chatSummaryEnabled = chatRuntimeObj.value("summary_enabled").toBool(true);
    m_chatSummaryCompressAfterTurns = chatRuntimeObj.value("summary_compress_after_turns").toInt(12);
    m_chatSummaryKeepRecentTurns = chatRuntimeObj.value("summary_keep_recent_turns").toInt(8);
    m_chatSummaryMaxChars = chatRuntimeObj.value("summary_max_chars").toInt(400);
    const int quarter = qMax(32, m_chatSummaryMaxChars / 4);
    m_memoryPreferencesMaxChars = chatRuntimeObj.value("memory_preferences_max_chars").toInt(quarter);
    m_memoryTasksMaxChars = chatRuntimeObj.value("memory_tasks_max_chars").toInt(quarter);
    m_memoryAvoidMaxChars = chatRuntimeObj.value("memory_avoid_max_chars").toInt(qMax(24, quarter - 20));
    m_memoryFactsMaxChars = chatRuntimeObj.value("memory_facts_max_chars").toInt(quarter);
    auto clampMem = [](int v) { return qMax(16, v); };
    m_memoryPreferencesMaxChars = clampMem(m_memoryPreferencesMaxChars);
    m_memoryTasksMaxChars = clampMem(m_memoryTasksMaxChars);
    m_memoryAvoidMaxChars = clampMem(m_memoryAvoidMaxChars);
    m_memoryFactsMaxChars = clampMem(m_memoryFactsMaxChars);
    if (m_chatContextTurns < 1) {
        m_chatContextTurns = 1;
    }
    if (m_chatMaxContextChars < 500) {
        m_chatMaxContextChars = 500;
    }
    if (m_chatSummaryKeepRecentTurns < 1) {
        m_chatSummaryKeepRecentTurns = 1;
    }
    if (m_chatSummaryCompressAfterTurns < m_chatSummaryKeepRecentTurns + 1) {
        m_chatSummaryCompressAfterTurns = m_chatSummaryKeepRecentTurns + 2;
    }
}

bool PetConfig::reloadChatRuntimeFromFile()
{
    if (m_configFilePath.isEmpty()) {
        return false;
    }

    QFile file(m_configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[配置] chat_runtime 同步读取失败:" << file.errorString();
        return false;
    }

    const QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[配置] chat_runtime 同步 JSON 无效:" << parseError.errorString();
        return false;
    }

    const QJsonObject chatRuntimeObj = getNestedObj(doc.object(), QStringLiteral("chat_runtime"));
    if (chatRuntimeObj.isEmpty()) {
        return false;
    }

    applyChatRuntimeObject(chatRuntimeObj);
    emit chatRuntimeSynced();
    return true;
}

QJsonObject PetConfig::getStructuredMemoryLimitsJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("preferences"), m_memoryPreferencesMaxChars);
    o.insert(QStringLiteral("tasks"), m_memoryTasksMaxChars);
    o.insert(QStringLiteral("avoid"), m_memoryAvoidMaxChars);
    o.insert(QStringLiteral("facts"), m_memoryFactsMaxChars);
    return o;
}

QString PetConfig::normalizeAnimationRelativePath(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (path.startsWith(QLatin1Char('/')))
        path = path.mid(1);
    return path;
}

double PetConfig::animationScaleForRelativePath(const QString& relativePath) const
{
    const QString k = normalizeAnimationRelativePath(relativePath);
    if (k.isEmpty())
        return 1.0;
    return m_animationScales.value(k, 1.0);
}
