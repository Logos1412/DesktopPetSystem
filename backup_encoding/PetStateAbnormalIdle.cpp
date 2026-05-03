#pragma execution_character_set("utf-8")

#include "PetStateAbnormalIdle.h"
#include "PetConfig.h"
#include <qdebug.h>

void PetStateAbnormalIdle::enter()
{
    qDebug() << "进入异常待机状态";

    // 断开旧定时器，避免重复触发
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateAbnormalIdle::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateAbnormalIdle::update);

    // 异常待机状态每秒更新一次
    m_updateTimer->start(1000);
}

void PetStateAbnormalIdle::update()
{
    // 获取配置文件中的衰减值
    PetConfig* config = PetConfig::getInstance();
    int hungerDecay = config->getHungerDecayAbnormal();    // 异常待机饱食衰减：1
    int energyDecay = config->getEnergyDecayAbnormal();     // 异常待机精力衰减：1
    int moodDecay = config->getMoodDecayAbnormal();        // 异常待机心情衰减：2

    // 执行属性衰减
    m_attr->changeHunger(-hungerDecay); 
    m_attr->changeEnergy(-energyDecay); 
    m_attr->changeMood(-moodDecay);

    qDebug() << "异常待机衰减：饱食" << m_attr->getHunger()
        << "精力" << m_attr->getEnergy()
        << "心情" << m_attr->getMood();

    // 检查状态切换
    checkStateSwitch();
}

void PetStateAbnormalIdle::exit()
{
    qDebug() << "退出异常待机状态";

    // 停止定时器
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateAbnormalIdle::update);
}

void PetStateAbnormalIdle::checkStateSwitch()
{
    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();

    // 精力<5 → 睡眠
    if (energy < 15) {
        m_fsm->switchState(PetStateType::Sleep);
        return;
    }

    // 饱食精力心情≥20 → 回到正常待机
    if (hunger >= 20 && energy >= 20 && mood >=20) {
        m_fsm->switchState(PetStateType::Idle);
        return;
    }

    qDebug() << "异常待机状态检查：当前饱食" << hunger << " 精力" << energy<<" 心情"<< mood;
}