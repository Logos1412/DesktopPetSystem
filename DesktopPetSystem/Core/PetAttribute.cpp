#pragma execution_character_set("utf-8")

#include "PetAttribute.h"
#include "PetConfig.h"

#include <qdebug.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QDir>

PetAttribute::PetAttribute(QObject* parent)
	:	QObject(parent)
{
	initAttribute();
}

void PetAttribute::initAttribute()
{
	PetConfig* config = PetConfig::getInstance();

	m_level = config->getDefaultLevel();
	m_exp = config->getDefaultExp();
	m_hunger = config->getDefaultHunger();
	m_energy = config->getDefaultEnergy();
	m_mood = config->getDefaultMood();
	m_coin = 0;

	qDebug() << "[属性初始化] 精力：" << m_energy;
}

void PetAttribute::resetToConfigDefaults()
{
	initAttribute();
	emit attributeChanged();
}

bool PetAttribute::loadFromFile(const QString& path)
{
	QFile file(path);
	if (!file.exists()) {
		return false;
	}

	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}

	QByteArray data = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (doc.isNull()) {
		return false;
	}

	QJsonObject obj = doc.object();
	m_level = obj.value("level").toInt(1);
	m_exp = obj.value("exp").toInt(0);
	m_hunger = obj.value("hunger").toInt(50);
	m_energy = obj.value("energy").toInt(60);
	m_mood = obj.value("mood").toInt(70);
	m_coin = obj.value("coin").toInt(0);

	emit attributeChanged();
	return true;
}

bool PetAttribute::saveToFile(const QString& path)
{
	QJsonObject obj;
	obj["level"] = m_level;
	obj["exp"] = m_exp;
	obj["hunger"] = m_hunger;
	obj["energy"] = m_energy;
	obj["mood"] = m_mood;
	obj["coin"] = m_coin;

	QJsonDocument doc(obj);

	QFileInfo fileInfo(path);
	QDir dir = fileInfo.absoluteDir();
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}

	file.write(doc.toJson());
	file.close();
	return true;
}

void PetAttribute::changeHunger(int delta)
{
	m_hunger = clampValue(m_hunger + delta);
	emit attributeChanged();
}

void PetAttribute::changeMood(int delta)
{
	m_mood = clampValue(m_mood + delta);
	emit attributeChanged();
}

void PetAttribute::changeEnergy(int delta)
{
	m_energy = clampValue(m_energy + delta);
	emit attributeChanged();
}

void PetAttribute::addExp(int exp, int* outLevelsGained)
{
	if (outLevelsGained)
		*outLevelsGained = 0;

	m_exp += exp;

	while (m_exp >= requiredExpForLevel(m_level)) {
		const int currentLevelRequiredExp = requiredExpForLevel(m_level);
		m_level++;
		m_exp -= currentLevelRequiredExp;
		if (outLevelsGained)
			(*outLevelsGained)++;
	}
	emit attributeChanged();
}

void PetAttribute::changeCoin(int delta) {
	m_coin += delta;
	if (m_coin < 0)
		m_coin = 0;
	emit attributeChanged();
}

// 限制属性范围（不在此处打 debug，统一由各处「结算」日志观察）
int PetAttribute::clampValue(int value)
{
	if (value > MAX_VALUE) {
		return MAX_VALUE;
	}
	if (value < MIN_VALUE) {
		return MIN_VALUE;
	}
	return value;
}

int PetAttribute::requiredExpForLevel(int level) const
{
    PetConfig* config = PetConfig::getInstance();
    const int base = config->getExpBase();
    const int growth = config->getExpGrowth();
    return base + growth * (level - 1);
}
