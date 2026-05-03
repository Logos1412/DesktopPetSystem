#pragma execution_character_set("utf-8")

#include "PetStateIdle.h"
#include "PetConfig.h"
#include <qdebug.h>

// 进入
void PetStateIdle::enter()
{
	qDebug() << "进入正常待机状态";

	// 播放待机动画
	qDebug() << "播放正常待机动画";

	// 先断开旧连接，再建立新连接
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateIdle::update);
	connect(m_updateTimer, &QTimer::timeout, this, &PetStateIdle::update);

	// 启动更新定时器
	m_updateTimer->start(1000); // 1000ms
}
//待机
void PetStateIdle::update()
{
	// 获取配置文件中的衰减值
	PetConfig* config = PetConfig::getInstance();
	int hungerDecay = config->getHungerDecayIdle();   // 正常待机饱食衰减：2
	int energyDecay = config->getEnergyDecayIdle();   // 正常待机精力衰减：2
	int moodDecay = config->getMoodDecayIdle();       // 正常待机心情衰减：1

	// 核心修复：传递负数增量
    m_attr->changeHunger(-hungerDecay);  // 饱食-1
    m_attr->changeEnergy(-energyDecay);  // 精力-1
    m_attr->changeMood(-moodDecay);      // 心情-2

	qDebug() << "正常待机衰减：饱食" << m_attr->getHunger()
		<< "精力" << m_attr->getEnergy()
		<< "心情" << m_attr->getMood();

	// 检查状态切换
	checkStateSwitch();
}

// 退出
void PetStateIdle::exit()
{
	qDebug() << "退出正常待机状态";

	// 停止计时器
	m_updateTimer->stop();
	disconnect(m_updateTimer, &QTimer::timeout, this, &PetStateIdle::update);

	// 停止待机动画
	qDebug() << "停止正常待机动画";
}

void PetStateIdle::checkStateSwitch()
{
	int hunger = m_attr->getHunger();
	int energy = m_attr->getEnergy();
	int mood = m_attr->getMood();

	// 精力值饱食度心情<20 
	if (hunger < 20 || energy < 20 || mood <20 ) {
		m_fsm->switchState(PetStateType::AbnormalIdle);
	}
	return;

	qDebug() << "待机状态：当前饱食度" << hunger << " 精力值" << energy<<" 精力值"<<mood;
}
