#pragma execution_character_set("utf-8")

#include "PetStateSleep.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "Config/PetConfig.h"

#include <QDebug>
#include <QTimer>

// 进入睡眠状态
void PetStateSleep::enter()
{
    qDebug() << "进入睡眠状态";
    resetPeriodicSettlementLog();
    m_sleepCount = 0;
    m_isSleeping = false;
    m_remSleepEnergy = 0;
    m_remSleepHunger = 0;

    // 先播放入睡动画（一轮结束后切到睡眠中循环）
    emit requestPlayAnimation("Sleep/Sleep.gif", true, true);

    /* 部分 GIF 在回绕到第 0 帧时 QMovie 不发 frameChanged，仅靠轮询/超时兜底 */
    QTimer::singleShot(10000, this, [this]() {
        if (m_fsm->currentState() != PetStateType::Sleep)
            return;
        if (m_isSleeping)
            return;
        qWarning() << "[睡眠] 入睡动画长时间未结束，强制切换熟睡循环";
        onFallAsleepIntroFinished();
    });

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
    const int energyRecoveryRate = m_config->getSleepEnergyRecoveryPerMinute();
    const int hungerDecayRate = m_config->getSleepHungerDecayPerMinute();

    ++m_sleepCount;

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
                             0,
                             0,
                             0);
}

// 退出睡眠状态
void PetStateSleep::exit()
{
    qDebug() << "退出睡眠状态";
    logExitSettlement(QStringLiteral("睡眠"));
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateSleep::update);
    m_sleepCount = 0;
    m_isSleeping = false;
}

void PetStateSleep::onFallAsleepIntroFinished()
{
    if (m_isSleeping)
        return;
    m_isSleeping = true;
    /* 入睡过渡只播一轮；熟睡循环 GIF，非单次 */
    emit requestPlayAnimation(QStringLiteral("Sleep/Sleeping.gif"), false, false);
    qDebug() << "[睡眠] 切换到睡眠中循环动画";
}

// 双击交互
void PetStateSleep::onDoubleClick()
{
    qDebug() << "[双击] 睡眠状态 - 唤醒宠物";
    m_fsm->changeState(PetStateType::Idle);
}
