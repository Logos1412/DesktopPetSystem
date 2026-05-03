#pragma execution_character_set("utf-8")

#include "PetStateWork.h"
#include <qdebug.h>

void PetStateWork::enter()
{
	qDebug() << "进入工作状态";
	qDebug() << "播放工作动画";

	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateWork::update);
	connect(m_updateTimer, &QTimer::timeout, this, &PetStateWork::update);
	m_updateTimer->start(1000);
}

void PetStateWork::update()
{
	m_attr->changeEnergy(-ENERGY_DECAY);  // 精力-5
	m_attr->changeHunger(-HUNGER_DECAY);  // 饱食-3
	m_attr->changeMood(-MOOD_DECAY);      // 心情-4
	m_attr->addExp(EXP_ADD);       // 经验+5
	m_attr->changeCoin(COIN_ADD);         // 金币+2

	qDebug() << "工作状态更新：精力-5，当前精力" << m_attr->getEnergy();
	qDebug() << "工作状态更新：饱食-3，当前饱食" << m_attr->getHunger();
	qDebug() << "工作状态更新：心情-4，当前心情" << m_attr->getMood();
	qDebug() << "工作状态更新：经验+5，当前经验" << m_attr->getExp();
	qDebug() << "工作状态更新：金币+2，当前金币" << m_attr->getCoin();

	checkStateSwitch();
}

void PetStateWork::exit()
{
	qDebug() << "退出工作状态";

	m_updateTimer->stop();
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateWork::update);

	qDebug() << "停止工作动画";
}

void PetStateWork::checkStateSwitch()
{
	int energy = m_attr->getEnergy();
	int hunger = m_attr->getHunger();

	if (energy < MIN_ENERGY) {
		qDebug() << "精力过低，自动退出工作状态";
		m_fsm->switchState(PetStateType::Idle);
		return;
	}
	if (hunger < MIN_HUNGER) {
		qDebug() << "饱食度过低，自动退出工作状态";
		m_fsm->switchState(PetStateType::Idle);
		return;
	}
}