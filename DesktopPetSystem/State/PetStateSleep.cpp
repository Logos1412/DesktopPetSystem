#pragma execution_character_set("utf-8")

#include "PetStateSleep.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "Config/PetConfig.h"

#include <QDebug>

// 进入睡眠状态
void PetStateSleep::enter()
{
    qDebug() << "进入睡眠状态";
    resetPeriodicSettlementLog();
    m_sleepCount = 0;
    m_isSleeping = false;
    m_remSleepEnergy = 0;
    m_remSleepHunger = 0;

    // 先播放入睡动画
    emit requestPlayAnimation("sleep/sleep.gif", true);

    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateSleep::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateSleep::update);
    m_updateTimer->start(1000);
}

// 状态更新
void PetStateSleep::update()
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[睡眠] 状态更新，睡眠次数:" << m_sleepCount;
    }

    // 获取配置
    int transitionDelay = m_config->getSleepTransitionDelay();
    const int energyRecoveryRate = m_config->getSleepEnergyRecoveryPerMinute();
    const int hungerDecayRate = m_config->getSleepHungerDecayPerMinute();

    m_sleepCount++;

    // 入睡过渡仅用于切换展示动画；属性从进入睡眠状态起每秒结算，不与动画绑定
    if (!m_isSleeping && m_sleepCount >= transitionDelay) {
        m_isSleeping = true;
        emit requestPlayAnimation("sleep/sleeping.gif", true);
        qDebug() << "[睡眠] 切换到睡眠中动画";
    }

    m_attr->changeEnergy(slicePerSecondFromRatePerMinute(energyRecoveryRate, m_remSleepEnergy));
    m_attr->changeHunger(-slicePerSecondFromRatePerMinute(hungerDecayRate, m_remSleepHunger));

    if (m_attr->getEnergy() >= m_config->getMaxValue()) {
        qDebug() << "[睡眠] 精力已满，自然醒";
        m_fsm->changeState(PetStateType::Idle);
        return;
    }

    maybeLogMinuteSettlement(QStringLiteral("睡眠"),
                             -m_config->getSleepHungerDecayPerMinute(),
                             m_config->getSleepEnergyRecoveryPerMinute(),
                             0);
}

// 退出睡眠状态
void PetStateSleep::exit()
{
    qDebug() << "退出睡眠状态";
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateSleep::update);
    m_sleepCount = 0;
    m_isSleeping = false;
}

// 双击交互
void PetStateSleep::onDoubleClick()
{
    qDebug() << "[双击] 睡眠状态 - 唤醒宠物";
    m_fsm->changeState(PetStateType::Idle);
}
