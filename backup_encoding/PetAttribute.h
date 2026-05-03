#pragma once
#pragma execution_character_set("utf-8")

#include <qobject.h>

class PetAttribute : public QObject
{
	Q_OBJECT

public:
	explicit PetAttribute(QObject* parent = nullptr);

	//初始化属性
	void initAttribute();

	/*
	修改属性（delta为增量：正数加，负数减）
	*/
	void changeHunger(int delta);
	void changeMood(int delta);
	void changeEnergy(int delta);
	void addExp(int exp);
	void changeCoin(int delta);

	/*
	读取属性
	*/
	int getHunger() const { return m_hunger; }
	int getMood() const { return m_mood; }
	int getEnergy() const { return m_energy; }
	int getLevel() const { return m_level; }
	int getExp() const { return m_exp; }
	int getCoin() const { return m_coin;}

private:
	//限制属性范围
	int clampValue(int value);

private:
	// 常量
	const int MAX_VALUE = 100; // 属性上限
	const int MIN_VALUE = 0;   // 属性下限

	// 成员变量（初始化移到initAttribute，构造函数仅初始化等级/经验）
	int m_hunger = 0;
	int m_mood = 0;
	int m_energy = 0;
	int m_level = 1;
	int m_exp = 0;
	int m_coin = 0;
};