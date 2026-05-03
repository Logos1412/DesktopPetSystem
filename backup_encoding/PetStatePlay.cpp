#pragma execution_character_set("utf-8")

#include "PetStatePlay.h"
#include <qdebug.h>

void PetStatePlay::enter()
{
	qDebug() << "进入玩耍状态";
	qDebug() << "播放玩耍动画";

	// 对齐你的定时器绑定逻辑
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStatePlay::update);
	connect(m_updateTimer, &QTimer::timeout, this, &PetStatePlay::update);
	m_updateTimer->start(1000);
}

void PetStatePlay::update()
{
	// 执行属性变化（和你的sleep类增量逻辑一致）
	m_attr->changeEnergy(-ENERGY_DECAY);  // 精力-2
	m_attr->changeHunger(-HUNGER_DECAY);  // 饱食-3
	m_attr->changeMood(MOOD_ADD);        // 心情+5

	// 日志格式对齐你的sleep类
	qDebug() << "玩耍状态更新：精力-2，当前精力" << m_attr->getEnergy();
	qDebug() << "玩耍状态更新：饱食-3，当前饱食" << m_attr->getHunger();
	qDebug() << "玩耍状态更新：心情+5，当前心情" << m_attr->getMood();

	checkStateSwitch();
}

void PetStatePlay::exit()
{
	qDebug() << "退出玩耍状态";

	// 对齐你的定时器停止逻辑
	m_updateTimer->stop();
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStatePlay::update);

	qDebug() << "停止玩耍动画";
}

void PetStatePlay::checkStateSwitch()
{
	int energy = m_attr->getEnergy();
	int mood = m_attr->getMood();

	// 退出条件：精力<10 或 心情满100
	if (energy < MIN_ENERGY) {
		qDebug() << "精力过低，自动退出玩耍状态";
		m_fsm->switchState(PetStateType::Idle);
		return;
	}
	if (mood >= MAX_MOOD) {
		qDebug() << "心情值已满，自动退出玩耍状态";
		m_fsm->switchState(PetStateType::Idle);
		return;
	}
}