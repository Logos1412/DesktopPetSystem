#pragma execution_character_set("utf-8")

#include "PetAttribute.h"
#include "PetConfig.h"

#include <qdebug.h>

PetAttribute::PetAttribute(QObject* parent)
	:	QObject(parent)
{
	// 构造时直接初始化属性（从配置读取）
	initAttribute();
}

//初始化属性
void PetAttribute::initAttribute()
{
	// 获取配置单例
	PetConfig* config = PetConfig::getInstance();

	// 从配置读取初始值（核心！）
	m_level = config->getDefaultLevel();
	m_exp = config->getDefaultExp();
	m_hunger = config->getDefaultHunger();  // 配置的50
	m_energy = config->getDefaultEnergy();  // 配置的60（不再是0！）
	m_mood = config->getDefaultMood();      // 配置的70

	qDebug() << "[属性初始化] 精力：" << m_energy; // 打印验证
}

//饱食修改
void PetAttribute::changeHunger(int delta)
{
	int oldVal = m_hunger;
	m_hunger = clampValue( m_hunger + delta);
	
	qDebug() << "饱食变化：" << oldVal << "->" << m_hunger;
}

//心情修改
void PetAttribute::changeMood(int delta)
{
	int oldVal = m_mood;
	m_mood = clampValue(m_mood + delta);

	qDebug() << "心情变化：" << oldVal << "->" << m_mood;
}

//精力修改
void PetAttribute::changeEnergy(int delta)
{
	int oldVal = m_energy;
	m_energy = clampValue(m_energy + delta);

	qDebug() << "精力变化：" << oldVal << "->" << m_energy;
}

//经验修改
void PetAttribute::addExp(int exp)
{
	m_exp += exp;
	qDebug() << "经验增加：" << exp
		<< "当前经验：" << m_exp;

	while (m_exp >= 100) {
		m_level++;
		m_exp -=100 ;
		qDebug() << "宠物升级！当前等级：" << m_level << "当前经验：" << m_exp;
	}
}

// 金币修改
void PetAttribute::changeCoin(int delta) {
	int oldVal = m_coin;
	m_coin += delta;
	// 金币不能为负
	if (m_coin < 0) m_coin = 0;

	qDebug() << "[属性变化] 金币：" << oldVal << "->" << m_coin;
}

//限制属性范围
int PetAttribute::clampValue(int value)
{
	if (value > MAX_VALUE) {
		qDebug() << "[属性越界] 值" << value << "超过上限" << MAX_VALUE << "，设置为" << MAX_VALUE;
		return MAX_VALUE;
	}
	if (value < MIN_VALUE) {
		qDebug() << "[属性越界] 值" << value << "低于下限" << MIN_VALUE << "，设置为" << MIN_VALUE;
		return MIN_VALUE;
	}
	return value;
}

