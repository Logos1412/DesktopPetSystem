#pragma execution_character_set("utf-8")

#include "PetStateWork.h"
#include "PetFSM.h"
#include "PetAttribute.h"

#include <QDebug>

// 进入工作状态
void PetStateWork::enter()
{
    qDebug() << "进入工作状态";
    resetPeriodicSettlementLog();
    m_remWorkEnergy = 0;
    m_remWorkHunger = 0;
    m_remWorkMood = 0;
    m_remWorkExp = 0;
    m_remWorkCoin = 0;

    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateWork::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateWork::update);
    m_updateTimer->start(1000);
}

// 状态更新
void PetStateWork::update()
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[工作] 状态更新";
    }

    m_attr->changeEnergy(-slicePerSecondFromRatePerMinute(m_config->getWorkEnergyDecayPerMinute(), m_remWorkEnergy));
    m_attr->changeHunger(-slicePerSecondFromRatePerMinute(m_config->getWorkHungerDecayPerMinute(), m_remWorkHunger));
    m_attr->changeMood(-slicePerSecondFromRatePerMinute(m_config->getWorkMoodDecayPerMinute(), m_remWorkMood));
    m_attr->addExp(slicePerSecondFromRatePerMinute(m_config->getWorkExpGainPerMinute(), m_remWorkExp));
    m_attr->changeCoin(slicePerSecondFromRatePerMinute(m_config->getWorkCoinGainPerMinute(), m_remWorkCoin));

    const int abnormal = m_config->getAbnormalThreshold();
    if (m_attr->getHunger() <= abnormal || m_attr->getEnergy() <= abnormal || m_attr->getMood() <= abnormal) {
        qDebug() << "[工作] 属性过低，切换到异常待机";
        m_fsm->changeState(PetStateType::AbnormalIdle);
        return;
    }

    maybeLogMinuteSettlement(QStringLiteral("工作"),
                             -m_config->getWorkHungerDecayPerMinute(),
                             -m_config->getWorkEnergyDecayPerMinute(),
                             -m_config->getWorkMoodDecayPerMinute());
}

// 退出工作状态
void PetStateWork::exit()
{
    qDebug() << "退出工作状态";
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateWork::update);
}
