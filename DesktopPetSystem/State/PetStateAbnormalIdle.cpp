#pragma execution_character_set("utf-8")

#include "PetStateAbnormalIdle.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h"

#include <QDebug>

// 进入异常待机状态
void PetStateAbnormalIdle::enter()
{
    qDebug() << "进入异常待机状态";
    resetPeriodicSettlementLog();

    m_remAbnormalHunger = 0;
    m_remAbnormalEnergy = 0;
    m_remAbnormalMood = 0;

    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateAbnormalIdle::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateAbnormalIdle::update);
    m_updateTimer->start(1000);
}

// 状态更新
void PetStateAbnormalIdle::update()
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[异常待机] 状态更新";
    }

    // 获取配置中的衰减值
    PetConfig* config = PetConfig::getInstance();

    m_attr->changeHunger(-slicePerSecondFromRatePerMinute(config->getHungerDecayAbnormal(), m_remAbnormalHunger));
    m_attr->changeEnergy(-slicePerSecondFromRatePerMinute(config->getEnergyDecayAbnormal(), m_remAbnormalEnergy));
    m_attr->changeMood(-slicePerSecondFromRatePerMinute(config->getMoodDecayAbnormal(), m_remAbnormalMood));

    // 检查状态切换
    checkStateSwitch();

    maybeLogMinuteSettlement(QStringLiteral("异常待机"),
                             -config->getHungerDecayAbnormal(),
                             -config->getEnergyDecayAbnormal(),
                             -config->getMoodDecayAbnormal());
}

// 退出异常待机状态
void PetStateAbnormalIdle::exit()
{
    qDebug() << "退出异常待机状态";
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateAbnormalIdle::update);
}

// 双击交互
void PetStateAbnormalIdle::onDoubleClick()
{
    AbnormalReason reason = getLowestAttribute();
    QString animationPath;

    switch (reason) {
    case AbnormalReason::Hungry:
        qDebug() << "[双击] 异常待机 - 饥饿动画";
        animationPath = "AbnormalIdle/double_click/Hungry.gif";
        break;
    case AbnormalReason::Sleepy:
        qDebug() << "[双击] 异常待机 - 困倦动画";
        animationPath = "AbnormalIdle/double_click/Sleepy.gif";
        break;
    case AbnormalReason::Sad:
        qDebug() << "[双击] 异常待机 - 悲伤动画";
        animationPath = "AbnormalIdle/double_click/Sad.gif";
        break;
    default:
        qDebug() << "[双击] 异常待机 - 默认动画";
        animationPath = "AbnormalIdle/AbnormalIdle.gif";
        break;
    }

    emit requestPlayAnimation(animationPath, false, true);
}

// 获取最低属性
AbnormalReason PetStateAbnormalIdle::getLowestAttribute()
{
    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();

    if (hunger <= energy && hunger <= mood) {
        return AbnormalReason::Hungry;
    }
    else if (energy <= hunger && energy <= mood) {
        return AbnormalReason::Sleepy;
    }
    else {
        return AbnormalReason::Sad;
    }
}

// 状态切换检查
void PetStateAbnormalIdle::checkStateSwitch()
{
    // 获取配置
    PetConfig* config = PetConfig::getInstance();
    int autoSleepEnergy = config->getAutoSleepEnergy();
    int recoveryThreshold = config->getRecoveryThreshold();

    // 检查属性是否恢复
    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();

    // 精力值过低时自动进入睡眠状态
    if (energy <= autoSleepEnergy) {
        qDebug() << "[状态切换] 精力过低(" << energy << ")，自动进入睡眠";
        m_fsm->changeState(PetStateType::Sleep);
        return;
    }

    if (hunger > recoveryThreshold && energy > recoveryThreshold && mood > recoveryThreshold) {
        qDebug() << "[状态切换] 属性恢复，切换回正常待机";
        m_fsm->changeState(PetStateType::Idle);
    }
}
