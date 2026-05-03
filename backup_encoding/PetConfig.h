#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>

// 宠物配置管理类（单例模式）
// 功能：读取JSON配置文件，封装嵌套节点读取逻辑，提供配置获取接口
class PetConfig : public QObject
{
    Q_OBJECT

private:
    // 私有构造函数（单例模式：禁止外部实例化）
    PetConfig(QObject* parent = nullptr) : QObject(parent) {}

    // 读取嵌套JSON对象
    QJsonObject getNestedObj(const QJsonObject& root, const QString& key);

    // 静态单例实例
    static PetConfig* m_instance;

    // 默认初始属性
    int m_defaultHunger = 50;    // 默认饱食度
    int m_defaultEnergy = 60;    // 默认精力值
    int m_defaultMood = 70;      // 默认心情值
    int m_defaultLevel = 1;         //默认等级
    int m_defaultExp = 0;           //默认经验

    // 正常待机状态衰减值
    int m_hungerDecayIdle = 2;   // 饱食度每秒衰减
    int m_energyDecayIdle = 2;   // 精力值每秒衰减
    int m_moodDecayIdle = 1;     // 心情值每秒衰减

    // 异常待机状态衰减值
    int m_hungerDecayAbnormal = 1; // 饱食度每秒衰减
    int m_energyDecayAbnormal = 1; // 精力值每秒衰减
    int m_moodDecayAbnormal = 2;   // 心情值每秒衰减

public:
    // 单例获取接口（全局唯一实例）
    static PetConfig* getInstance();

    // 加载配置文件（核心接口）
    bool loadConfig(const QString& path);

    // 默认初始属性
    int getDefaultHunger() const { return m_defaultHunger; }
    int getDefaultEnergy() const { return m_defaultEnergy; }
    int getDefaultMood() const { return m_defaultMood; }
    int getDefaultLevel() const { return m_defaultLevel; }
    int getDefaultExp() const { return m_defaultExp; }

    // 正常待机衰减配置
    int getHungerDecayIdle() const { return m_hungerDecayIdle; }
    int getEnergyDecayIdle() const { return m_energyDecayIdle; }
    int getMoodDecayIdle() const { return m_moodDecayIdle; }

    // 异常待机衰减配置
    int getHungerDecayAbnormal() const { return m_hungerDecayAbnormal; }
    int getEnergyDecayAbnormal() const { return m_energyDecayAbnormal; }
    int getMoodDecayAbnormal() const { return m_moodDecayAbnormal; }

};