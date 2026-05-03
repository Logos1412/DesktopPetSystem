#pragma execution_character_set("utf-8")

#include "PetConfig.h"

// 静态单例实例初始化（全局唯一）
PetConfig* PetConfig::m_instance = nullptr;

// 单例获取接口实现 
PetConfig* PetConfig::getInstance() {
    // 懒加载：第一次调用时才创建实例
    if (m_instance == nullptr) {
        m_instance = new PetConfig();
    }
    return m_instance;
}

// 嵌套JSON对象读取工具函数 
QJsonObject PetConfig::getNestedObj(const QJsonObject& root, const QString& key) {
    // 检查节点是否存在
    if (!root.contains(key)) {
        qWarning() << "[配置警告] 缺少" << key << "节点，使用默认值";
        return QJsonObject(); // 返回空对象，toInt会自动使用默认值
    }

    // 检查节点类型是否为Object
    QJsonValue value = root[key];
    if (!value.isObject()) {
        qWarning() << "[配置警告]" << key << "节点不是JSON对象，使用默认值";
        return QJsonObject();
    }

    // 返回嵌套的JSON对象
    return value.toObject();
}

//  配置文件加载核心逻辑
bool PetConfig::loadConfig(const QString& path) {
    // 打印加载路径
    qDebug() << "[配置加载] 尝试读取：" << path;

    // 打开文件
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "[配置错误] 文件打开失败：" << file.errorString();
        return false;
    }

    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close(); // 及时关闭文件

    // 打印读取到的内容（确认文件非空）
    qDebug() << "[配置加载] 读取到内容：" << jsonData;

    // 解析JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    // 检查解析错误
    if (doc.isNull() || !doc.isObject()) {
        qCritical() << "[配置错误] JSON解析失败：" << parseError.errorString()
            << "错误位置：" << parseError.offset;
        return false;
    }

    // 读取嵌套节点（核心优化：调用工具函数）
    QJsonObject rootObj = doc.object();
    QJsonObject attrObj = getNestedObj(rootObj, "attributes"); // 读取attributes节点

    // 解析配置项（从嵌套的attrObj读取）
    // 默认初始属性
    m_defaultHunger = attrObj.value("default_hunger").toInt(50);
    m_defaultEnergy = attrObj.value("default_energy").toInt(60);
    m_defaultMood = attrObj.value("default_mood").toInt(70);
    m_defaultLevel = attrObj.value("default_level").toInt(1);
    m_defaultExp = attrObj.value("default_exp").toInt(0);

    // 正常待机衰减
    m_hungerDecayIdle = attrObj.value("hunger_decay_idle").toInt(2);
    m_energyDecayIdle = attrObj.value("energy_decay_idle").toInt(2);
    m_moodDecayIdle = attrObj.value("mood_decay_idle").toInt(1);

    // 异常待机衰减
    m_hungerDecayAbnormal = attrObj.value("hunger_decay_abnormal").toInt(1);
    m_energyDecayAbnormal = attrObj.value("energy_decay_abnormal").toInt(1);
    m_moodDecayAbnormal = attrObj.value("mood_decay_abnormal").toInt(2);


    // 打印加载结果
    qDebug() << "[配置加载成功] =========";
    qDebug() << "默认属性：饱食=" << m_defaultHunger << "精力=" << m_defaultEnergy << "心情=" << m_defaultMood;
    qDebug() << "正常待机衰减：饱食=" << m_hungerDecayIdle << "精力=" << m_energyDecayIdle << "心情=" << m_moodDecayIdle;
    qDebug() << "异常待机衰减：饱食=" << m_hungerDecayAbnormal << "精力=" << m_energyDecayAbnormal << "心情=" << m_moodDecayAbnormal;
    qDebug() << "========================";

    return true;
}