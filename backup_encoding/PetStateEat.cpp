#pragma execution_character_set("utf-8")

#include "PetStateEat.h"
#include <qdebug.h>

void PetStateEat::enter()
{
    qDebug() << "进入进食状态";
    qDebug() << "播放进食动画";

    m_eatDuration = 0;

    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateEat::update);
    connect(m_updateTimer, &QTimer::timeout, this, &PetStateEat::update);

    m_updateTimer->start(1000);
}

void PetStateEat::update()
{
    m_eatDuration++;

    qDebug() << "进食中... 已持续" << m_eatDuration << "秒";

    if (m_eatDuration >= EAT_DURATION) {
        m_attr->changeHunger(m_hungerBonus);
        m_attr->changeMood(m_moodBonus);
        m_attr->changeEnergy(m_energyBonus);

        qDebug() << "进食完成！饱食+" << m_hungerBonus 
                 << "心情+" << m_moodBonus 
                 << "精力+" << m_energyBonus;

        checkStateSwitch();
    }
}

void PetStateEat::exit()
{
    qDebug() << "退出进食状态";

    m_updateTimer->stop();
    disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateEat::update);
}

void PetStateEat::setFoodBonus(int hungerBonus, int moodBonus, int energyBonus)
{
    m_hungerBonus = hungerBonus;
    m_moodBonus = moodBonus;
    m_energyBonus = energyBonus;
}

void PetStateEat::checkStateSwitch()
{
    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();

    if (hunger < 20 || energy < 20 || mood < 20) {
        m_fsm->switchState(PetStateType::AbnormalIdle);
    } else {
        m_fsm->switchState(PetStateType::Idle);
    }
}
