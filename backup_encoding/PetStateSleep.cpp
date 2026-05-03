#pragma execution_character_set("utf-8")

#include "PetStateSleep.h"
#include <qdebug.h>

void PetStateSleep::enter()
{
	qDebug() << "进入休眠状态";
	qDebug() << "播放休眠动画";

	// 对齐其他手动状态的定时器绑定逻辑
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateSleep::update);
	connect(m_updateTimer, &QTimer::timeout, this, &PetStateSleep::update);
	m_updateTimer->start(1000);
}

void PetStateSleep::update()
{
	// 保留你原有的精力恢复逻辑，补充饱食/心情衰减（更符合业务逻辑）
	m_attr->changeEnergy(ENERGY_ADD);        // 精力+5（和你原有逻辑一致）
	m_attr->changeHunger(-HUNGER_DECAY);     // 饱食-1（新增）
	m_attr->changeMood(MOOD_ADD);         // 心情-1（新增）

	// 日志格式对齐其他手动状态，同时保留你的核心信息
	qDebug() << "休眠状态更新：精力+5，当前精力值" << m_attr->getEnergy();
	qDebug() << "休眠状态更新：饱食-1，当前饱食值" << m_attr->getHunger();
	qDebug() << "休眠状态更新：心情-1，当前心情值" << m_attr->getMood();

	// 状态切换检查（和其他状态统一调用）
	checkStateSwitch();
}

void PetStateSleep::exit()
{
	qDebug() << "退出休眠状态";

	// 对齐其他手动状态的定时器停止逻辑
	m_updateTimer->stop();
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateSleep::update);

	qDebug() << "停止休眠动画";
}

// 保留你原有的主动唤醒逻辑
void PetStateSleep::wakeUp()
{
	qDebug() << "用户主动唤醒宠物";
	m_fsm->switchState(PetStateType::Idle);
}

void PetStateSleep::checkStateSwitch()
{
	int energy = m_attr->getEnergy();
	// 保留你原有的精力满退出逻辑
	if (energy >= MAX_ENERGY) {
		qDebug() << "精力值已满，自动退出休眠";
		m_fsm->switchState(PetStateType::Idle);
	}
}