#pragma execution_character_set("utf-8")

#include "PetState.h"

#include <QStringList>

#include <QDebug>

QString PetState::formatSettlementAttrDelta(const QString& label, int delta)
{
    return label + ((delta > 0) ? QStringLiteral("+") : QString()) + QString::number(delta);
}

void PetState::resetPeriodicSettlementLog()
{
    m_periodicSettlementTicks = 0;
    if (m_attr) {
        m_settlementBaseHunger = m_attr->getHunger();
        m_settlementBaseEnergy = m_attr->getEnergy();
        m_settlementBaseMood = m_attr->getMood();
        m_settlementBaseExp = m_attr->getExp();
        m_settlementBaseLevel = m_attr->getLevel();
        m_settlementBaseCoin = m_attr->getCoin();
    }
}

void PetState::maybeLogMinuteSettlement(const QString& stateLabel,
                                       int expectedDh,
                                       int expectedDe,
                                       int expectedDm,
                                       int expectedDexp,
                                       int expectedDcoin)
{
    if (!m_attr) {
        return;
    }
    ++m_periodicSettlementTicks;
    if (m_periodicSettlementTicks < 60) {
        return;
    }
    m_periodicSettlementTicks = 0;

    const int h = m_attr->getHunger();
    const int e = m_attr->getEnergy();
    const int m = m_attr->getMood();
    const int x = m_attr->getExp();
    const int lv = m_attr->getLevel();
    const int levelGain = lv - m_settlementBaseLevel;

    QStringList deltas;
    deltas << formatSettlementAttrDelta(QStringLiteral("饱食"), expectedDh);
    deltas << formatSettlementAttrDelta(QStringLiteral("精力"), expectedDe);
    deltas << formatSettlementAttrDelta(QStringLiteral("心情"), expectedDm);
    if (expectedDexp != 0)
        deltas << formatSettlementAttrDelta(QStringLiteral("经验"), expectedDexp);
    if (expectedDcoin != 0)
        deltas << formatSettlementAttrDelta(QStringLiteral("金币"), expectedDcoin);

    const int c = m_attr->getCoin();

    qDebug() << "[结算]" << stateLabel << "| 60s |" << deltas.join(QStringLiteral(" "))
             << "| 结算前" << m_settlementBaseHunger << m_settlementBaseEnergy << m_settlementBaseMood
             << m_settlementBaseExp << m_settlementBaseCoin << QStringLiteral("→ 结算后") << h << e << m << x << c
             << QStringLiteral("| 升级：") << levelGain;

    m_settlementBaseHunger = h;
    m_settlementBaseEnergy = e;
    m_settlementBaseMood = m;
    m_settlementBaseExp = x;
    m_settlementBaseLevel = lv;
    m_settlementBaseCoin = c;
}

int PetState::slicePerSecondFromRatePerMinute(int ratePerMinute, int& remainder)
{
    remainder += ratePerMinute;
    const int slice = remainder / 60;
    remainder %= 60;
    return slice;
}
