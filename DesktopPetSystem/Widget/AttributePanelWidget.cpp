#pragma execution_character_set("utf-8")

#include "AttributePanelWidget.h"
#include "PetAttribute.h"
#include <QScreen>
#include <QApplication>
#include <QDebug>

AttributePanelWidget::AttributePanelWidget(PetAttribute* attr, QWidget* parent)
    : QWidget(parent), m_attr(attr)
{
    initStyle();
    initUI();
}

void AttributePanelWidget::initStyle()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setFixedWidth(280);
    setMouseTracking(true);

    setStyleSheet(
        "QWidget {"
        "    background-color: white;"
        "    border: 1px solid #aaa;"
        "    border-radius: 8px;"
        "}"
        "QLabel {"
        "    font-size: 12px;"
        "    color: #333;"
        "    background: transparent;"
        "}"
        "QProgressBar {"
        "    border: 1px solid #ccc;"
        "    border-radius: 4px;"
        "    text-align: center;"
        "    font-size: 10px;"
        "    background: #f0f0f0;"
        "}"
        "QProgressBar::chunk {"
        "    border-radius: 3px;"
        "}"
    );
}

void AttributePanelWidget::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    QLabel* titleLabel = new QLabel(u8"宠物属性面板", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2196F3;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QHBoxLayout* levelLayout = new QHBoxLayout();
    m_levelLabel = new QLabel(u8"等级: 1", this);
    m_expLabel = new QLabel(u8"经验: 0", this);
    m_coinLabel = new QLabel(u8"金币: 0", this);
    levelLayout->addWidget(m_levelLabel);
    levelLayout->addStretch();
    levelLayout->addWidget(m_expLabel);
    levelLayout->addStretch();
    levelLayout->addWidget(m_coinLabel);
    mainLayout->addLayout(levelLayout);

    auto createAttrRow = [this, mainLayout](const QString& name, QProgressBar*& bar, QLabel*& valueLabel, const QString& color) {
        QHBoxLayout* layout = new QHBoxLayout();
        
        QLabel* nameLabel = new QLabel(name, this);
        nameLabel->setFixedWidth(50);
        
        bar = new QProgressBar(this);
        bar->setRange(0, 100);
        bar->setFixedHeight(18);
        bar->setFormat("%p%");
        bar->setStyleSheet(
            "QProgressBar::chunk { background-color: " + color + "; }"
        );
        
        valueLabel = new QLabel("100", this);
        valueLabel->setFixedWidth(35);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        layout->addWidget(nameLabel);
        layout->addWidget(bar);
        layout->addWidget(valueLabel);
        
        mainLayout->addLayout(layout);
    };

    createAttrRow(u8"饱食度", m_hungerBar, m_hungerValue, "#FF9800");
    createAttrRow(u8"精力", m_energyBar, m_energyValue, "#4CAF50");
    createAttrRow(u8"心情", m_moodBar, m_moodValue, "#E91E63");

    refreshData();
    adjustSize();
    setFixedHeight(height());
}

void AttributePanelWidget::showAtPos(const QPoint& pos)
{
    refreshData();
    
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    
    QPoint finalPos = pos;
    if (finalPos.x() + width() > screenRect.width()) {
        finalPos.setX(screenRect.width() - width());
    }
    if (finalPos.y() + height() > screenRect.height()) {
        finalPos.setY(screenRect.height() - height());
    }
    
    move(finalPos);
    show();
    raise();
    activateWindow();
}

void AttributePanelWidget::refreshData()
{
    if (!m_attr) return;

    m_levelLabel->setText(u8"等级: " + QString::number(m_attr->getLevel()));
    m_expLabel->setText(u8"经验: " + QString::number(m_attr->getExp()));
    m_coinLabel->setText(u8"金币: " + QString::number(m_attr->getCoin()));

    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();

    m_hungerBar->setValue(hunger);
    m_energyBar->setValue(energy);
    m_moodBar->setValue(mood);

    m_hungerValue->setText(QString::number(hunger));
    m_energyValue->setText(QString::number(energy));
    m_moodValue->setText(QString::number(mood));
}

void AttributePanelWidget::enterEvent(QEvent* event)
{
    Q_UNUSED(event);
    emit mouseEntered();
}

void AttributePanelWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    emit mouseLeft();
}
