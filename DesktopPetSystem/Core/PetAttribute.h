#pragma once
#pragma execution_character_set("utf-8")

#include <qobject.h>

class PetAttribute : public QObject
{
	Q_OBJECT

public:
	explicit PetAttribute(QObject* parent = nullptr);

	void initAttribute();
	/** 按 pet_config 中的 default_* 恢复属性与金币 0，并发 attributeChanged */
	void resetToConfigDefaults();
	bool loadFromFile(const QString& path);
	bool saveToFile(const QString& path);

	void changeHunger(int delta);
	void changeMood(int delta);
	void changeEnergy(int delta);
	/** outLevelsGained 非空时写入本次因加经验而提升的等级数（可能 >1） */
	void addExp(int exp, int* outLevelsGained = nullptr);
	void changeCoin(int delta);

	int getHunger() const { return m_hunger; }
	int getMood() const { return m_mood; }
	int getEnergy() const { return m_energy; }
	int getLevel() const { return m_level; }
	int getExp() const { return m_exp; }
	int getCoin() const { return m_coin;}

	/** 由存档加载等非 setter 路径触发界面刷新 */
	void notifyAttributeChanged() { emit attributeChanged(); }

signals:
	void attributeChanged();

private:
	int clampValue(int value);
    int requiredExpForLevel(int level) const;

private:
	const int MAX_VALUE = 100;
	const int MIN_VALUE = 0;

	int m_hunger = 0;
	int m_mood = 0;
	int m_energy = 0;
	int m_level = 1;
	int m_exp = 0;
	int m_coin = 0;

    friend class PetDatabase;
};
