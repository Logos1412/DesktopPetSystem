#pragma execution_character_set("utf-8")

#include "PetStateIdle.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h"

#include <QDebug>

// 进入正常待机状态
void PetStateIdle::enter()
{
    qDebug() << "进入正常待机状态";
    resetPeriodicSettlementLog();
    m_remIdleHunger = 0;
    m_remIdleEnergy = 0;
    m_remIdleMood = 0;
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateIdle::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateIdle::update);
    m_updateTimer->start(1000);
}

// 状态更新
void PetStateIdle::update()
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[正常待机] 状态更新";
    }

    // 获取配置中的衰减值
    PetConfig* config = PetConfig::getInstance();

    /* 配置文件为每分钟衰减；每秒 tick：累加后对 60 取整分摊 */
    m_attr->changeHunger(-slicePerSecondFromRatePerMinute(config->getHungerDecayIdle(), m_remIdleHunger));
    m_attr->changeEnergy(-slicePerSecondFromRatePerMinute(config->getEnergyDecayIdle(), m_remIdleEnergy));
    m_attr->changeMood(-slicePerSecondFromRatePerMinute(config->getMoodDecayIdle(), m_remIdleMood));

    // 检查状态切换
    checkStateSwitch();

    maybeLogMinuteSettlement(QStringLiteral("正常待机"),
                             -config->getHungerDecayIdle(),
                             -config->getEnergyDecayIdle(),
                             -config->getMoodDecayIdle());
}

// 退出正常待机状态
void PetStateIdle::exit()
{
    qDebug() << "退出正常待机状态";
    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateIdle::update);
}

// 双击交互
void PetStateIdle::onDoubleClick()
{
    qDebug() << "[双击] 正常待机 - 播放开心动画";
    emit requestPlayAnimation("idle/happy.gif", false);
}

// 状态切换检查
void PetStateIdle::checkStateSwitch()
{
    // 获取配置
    PetConfig* config = PetConfig::getInstance();
    int threshold = config->getAbnormalThreshold();

    // 检查属性是否过低
    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();

    if (hunger <= threshold || energy <= threshold || mood <= threshold) {
        qDebug() << "[状态切换] 属性过低，切换到异常待机";
        m_fsm->changeState(PetStateType::AbnormalIdle);
    }
}
