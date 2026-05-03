#pragma execution_character_set("utf-8")

#include "PetStateEat.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h"

#include <QDebug>

namespace {
constexpr int kEatFinishDelayMs = 5000;
}

void PetStateEat::setFood(const QString& foodId)
{
    PetConfig* config = PetConfig::getInstance();
    if (config->hasFood(foodId)) {
        setFood(config->getFood(foodId));
        return;
    }
    m_hasFood = false;
}

void PetStateEat::setFood(const FoodData& food)
{
    m_currentFood = food;
    m_hasFood = !food.id.isEmpty();
}

void PetStateEat::enter()
{
    if (!m_hasFood) {
        qWarning() << "[进食] 未选择有效食物，回到待机状态";
        m_fsm->changeState(PetStateType::Idle);
        return;
    }

    const FoodData& f = m_currentFood;

    const int h0 = m_attr->getHunger();
    const int e0 = m_attr->getEnergy();
    const int m0 = m_attr->getMood();

    m_attr->changeHunger(f.hunger);
    m_attr->changeEnergy(f.energy);
    m_attr->changeMood(f.mood);
    if (f.exp != 0)
        m_attr->addExp(f.exp);

    qDebug() << "[结算]" << QStringLiteral("喂食") << "|" << f.name << "|"
             << formatSettlementAttrDelta(QStringLiteral("饱食"), f.hunger)
             << formatSettlementAttrDelta(QStringLiteral("精力"), f.energy)
             << formatSettlementAttrDelta(QStringLiteral("心情"), f.mood)
             << "| 结算前" << h0 << e0 << m0 << QStringLiteral("→ 结算后")
             << m_attr->getHunger() << m_attr->getEnergy() << m_attr->getMood();

    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, nullptr);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateEat::finishEating);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->start(kEatFinishDelayMs);
}

void PetStateEat::update() {}

void PetStateEat::finishEating()
{
    m_fsm->changeState(PetStateType::Idle);
}

void PetStateEat::exit()
{
    qDebug() << "退出进食状态";
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateEat::finishEating);
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateEat::update);
    m_updateTimer->stop();
    m_hasFood = false;
}
