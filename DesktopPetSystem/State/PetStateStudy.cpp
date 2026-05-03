#pragma execution_character_set("utf-8")

#include "PetStateStudy.h"
#include "PetFSM.h"
#include "PetAttribute.h"

#include <QDebug>

// 进入学习状态
void PetStateStudy::enter()
{
    qDebug() << "进入学习状态";
    resetPeriodicSettlementLog();
    m_remStudyEnergy = 0;
    m_remStudyHunger = 0;
    m_remStudyMood = 0;
    m_remStudyExp = 0;

    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateStudy::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateStudy::update);
    m_updateTimer->start(1000);
}

// 状态更新
void PetStateStudy::update()
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[学习] 状态更新";
    }

    m_attr->changeEnergy(-slicePerSecondFromRatePerMinute(m_config->getStudyEnergyDecayPerMinute(), m_remStudyEnergy));
    m_attr->changeHunger(-slicePerSecondFromRatePerMinute(m_config->getStudyHungerDecayPerMinute(), m_remStudyHunger));
    m_attr->changeMood(-slicePerSecondFromRatePerMinute(m_config->getStudyMoodDecayPerMinute(), m_remStudyMood));
    m_attr->addExp(slicePerSecondFromRatePerMinute(m_config->getStudyExpGainPerMinute(), m_remStudyExp));

    const int abnormal = m_config->getAbnormalThreshold();
    if (m_attr->getHunger() <= abnormal || m_attr->getEnergy() <= abnormal || m_attr->getMood() <= abnormal) {
        qDebug() << "[学习] 属性过低，切换到异常待机";
        m_fsm->changeState(PetStateType::AbnormalIdle);
        return;
    }

    maybeLogMinuteSettlement(QStringLiteral("学习"),
                             -m_config->getStudyHungerDecayPerMinute(),
                             -m_config->getStudyEnergyDecayPerMinute(),
                             -m_config->getStudyMoodDecayPerMinute());
}

// 退出学习状态
void PetStateStudy::exit()
{
    qDebug() << "退出学习状态";
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateStudy::update);
}
