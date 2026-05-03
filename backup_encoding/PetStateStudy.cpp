#pragma execution_character_set("utf-8")

#include "PetStateStudy.h"
#include <qdebug.h>

void PetStateStudy::enter()
{
	qDebug() << "进入学习状态";
	qDebug() << "播放学习动画";

	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateStudy::update);
	connect(m_updateTimer, &QTimer::timeout, this, &PetStateStudy::update);
	m_updateTimer->start(1000);
}

void PetStateStudy::update()
{
	m_attr->changeEnergy(-ENERGY_DECAY);  // 精力-4
	m_attr->changeHunger(-HUNGER_DECAY);  // 饱食-2
	m_attr->changeMood(-MOOD_DECAY);      // 心情-2
	m_attr->addExp(EXP_ADD);       // 经验+3

	qDebug() << "学习状态更新：精力-4，当前精力" << m_attr->getEnergy();
	qDebug() << "学习状态更新：饱食-2，当前饱食" << m_attr->getHunger();
	qDebug() << "学习状态更新：心情-2，当前心情" << m_attr->getMood();
	qDebug() << "学习状态更新：经验+3，当前经验" << m_attr->getExp();

	checkStateSwitch();
}

void PetStateStudy::exit()
{
	qDebug() << "退出学习状态";

	m_updateTimer->stop();
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateStudy::update);

	qDebug() << "停止学习动画";
}

void PetStateStudy::checkStateSwitch()
{
	int energy = m_attr->getEnergy();

	if (energy < MIN_ENERGY) {
		qDebug() << "精力过低，自动退出学习状态";
		m_fsm->switchState(PetStateType::Idle);
		return;
	}
}