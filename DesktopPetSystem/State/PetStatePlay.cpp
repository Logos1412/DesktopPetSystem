#pragma execution_character_set("utf-8")

#include "PetStatePlay.h"
#include "PetFSM.h"
#include "PetAttribute.h"

#include <QDebug>

// 进入玩耍状态
void PetStatePlay::enter()
{
    qDebug() << "进入玩耍状态";
    resetPeriodicSettlementLog();
    m_remPlayMood = 0;
    m_remPlayEnergy = 0;
    m_remPlayHunger = 0;
    m_remPlayExp = 0;

    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStatePlay::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStatePlay::update);
    m_updateTimer->start(1000);
}

// 状态更新
void PetStatePlay::update()
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[玩耍] 状态更新";
    }

    m_attr->changeMood(slicePerSecondFromRatePerMinute(m_config->getPlayMoodIncreasePerMinute(), m_remPlayMood));
    m_attr->changeEnergy(-slicePerSecondFromRatePerMinute(m_config->getPlayEnergyDecayPerMinute(), m_remPlayEnergy));
    m_attr->changeHunger(-slicePerSecondFromRatePerMinute(m_config->getPlayHungerDecayPerMinute(), m_remPlayHunger));
    m_attr->addExp(slicePerSecondFromRatePerMinute(m_config->getPlayExpGainPerMinute(), m_remPlayExp));

    const int abnormal = m_config->getAbnormalThreshold();
    if (m_attr->getHunger() <= abnormal || m_attr->getEnergy() <= abnormal || m_attr->getMood() <= abnormal) {
        qDebug() << "[玩耍] 属性过低，切换到异常待机";
        m_fsm->changeState(PetStateType::AbnormalIdle);
        return;
    }

    maybeLogMinuteSettlement(QStringLiteral("玩耍"),
                             -m_config->getPlayHungerDecayPerMinute(),
                             -m_config->getPlayEnergyDecayPerMinute(),
                             m_config->getPlayMoodIncreasePerMinute());
}

// 退出玩耍状态
void PetStatePlay::exit()
{
    qDebug() << "退出玩耍状态";
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStatePlay::update);
}
