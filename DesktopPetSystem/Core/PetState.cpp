#pragma execution_character_set("utf-8")

#include "PetState.h"

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
    }
}

void PetState::maybeLogMinuteSettlement(const QString& stateLabel,
                                       int expectedDh,
                                       int expectedDe,
                                       int expectedDm)
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

    qDebug() << "[结算]" << stateLabel << "| 60s |"
             << formatSettlementAttrDelta(QStringLiteral("饱食"), expectedDh)
             << formatSettlementAttrDelta(QStringLiteral("精力"), expectedDe)
             << formatSettlementAttrDelta(QStringLiteral("心情"), expectedDm)
             << "| 结算前" << m_settlementBaseHunger << m_settlementBaseEnergy << m_settlementBaseMood
             << QStringLiteral("→ 结算后") << h << e << m;

    m_settlementBaseHunger = h;
    m_settlementBaseEnergy = e;
    m_settlementBaseMood = m;
}

int PetState::slicePerSecondFromRatePerMinute(int ratePerMinute, int& remainder)
{
    remainder += ratePerMinute;
    const int slice = remainder / 60;
    remainder %= 60;
    return slice;
}
