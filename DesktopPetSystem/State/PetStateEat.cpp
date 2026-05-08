#pragma execution_character_set("utf-8")

#include "PetStateEat.h"
#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h"

#include <QStringList>

#include <QDebug>

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
    const int exp0 = m_attr->getExp();
    const int coin0 = m_attr->getCoin();

    m_attr->changeHunger(f.hunger);
    m_attr->changeEnergy(f.energy);
    m_attr->changeMood(f.mood);
    int levelGain = 0;
    if (f.exp != 0)
        m_attr->addExp(f.exp, &levelGain);

    QStringList deltas;
    deltas << formatSettlementAttrDelta(QStringLiteral("饱食"), f.hunger);
    deltas << formatSettlementAttrDelta(QStringLiteral("精力"), f.energy);
    deltas << formatSettlementAttrDelta(QStringLiteral("心情"), f.mood);
    if (f.exp != 0)
        deltas << formatSettlementAttrDelta(QStringLiteral("经验"), f.exp);

    qDebug() << "[结算]" << QStringLiteral("喂食") << "|" << f.name << "|" << deltas.join(QStringLiteral(" "))
             << "| 结算前" << h0 << e0 << m0 << exp0 << coin0 << QStringLiteral("→ 结算后")
             << m_attr->getHunger() << m_attr->getEnergy() << m_attr->getMood() << m_attr->getExp()
             << m_attr->getCoin() << QStringLiteral("| 升级：") << levelGain;

    /* 结束时机：进食 GIF 播完一轮（见 PetWidget + notifyEatAnimationFinished） */
}

void PetStateEat::update() {}

void PetStateEat::exit()
{
    qDebug() << "退出进食状态";
    m_hasFood = false;
}
