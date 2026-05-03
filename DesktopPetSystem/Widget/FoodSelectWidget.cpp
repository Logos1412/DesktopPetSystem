#pragma execution_character_set("utf-8")

#include "FoodSelectWidget.h"
#include "PetAttribute.h"
#include "PetConfig.h"

#include <QScrollArea>
#include <QFrame>

FoodSelectWidget::FoodSelectWidget(PetAttribute* attr, QWidget* parent)
    : QWidget(nullptr), m_attr(attr)
{
    initStyle();
    initUI();
}

void FoodSelectWidget::initStyle()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::Popup);
    setFixedSize(220, 280);
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
        "}"
        "QPushButton {"
        "    border: 1px solid #ccc;"
        "    background: #f5f5f5;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background: #e0e0e0;"
        "}"
        "QPushButton:disabled {"
        "    background: #f0f0f0;"
        "    color: #999;"
        "}"
    );
}

void FoodSelectWidget::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    QLabel* titleLabel = new QLabel(u8"选择食物", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2196F3;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background: #ddd;");
    mainLayout->addWidget(line);

    QWidget* foodContainer = new QWidget(this);
    m_foodLayout = new QVBoxLayout(foodContainer);
    m_foodLayout->setContentsMargins(0, 0, 0, 0);
    m_foodLayout->setSpacing(6);

    PetConfig* config = PetConfig::getInstance();
    QMap<QString, FoodData> foods = config->getFoods();

    for (auto it = foods.begin(); it != foods.end(); ++it) {
        QWidget* item = createFoodItem(it.value());
        m_foodLayout->addWidget(item);
    }

    foodContainer->setLayout(m_foodLayout);
    mainLayout->addWidget(foodContainer);

    mainLayout->addStretch();

    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background: #ddd;");
    mainLayout->addWidget(line2);

    m_coinLabel = new QLabel(u8"金币: 0", this);
    m_coinLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #FF9800;");
    m_coinLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_coinLabel);

    updateCoinDisplay();
}

QWidget* FoodSelectWidget::createFoodItem(const FoodData& food)
{
    QWidget* item = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(item);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    QLabel* nameLabel = new QLabel(food.name, item);
    nameLabel->setFixedWidth(50);
    nameLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(nameLabel);

    QString effectText;
    if (food.hunger > 0) effectText += u8"饱食+" + QString::number(food.hunger) + " ";
    if (food.energy > 0) effectText += u8"精力+" + QString::number(food.energy) + " ";
    if (food.mood > 0) effectText += u8"心情+" + QString::number(food.mood);
    
    QLabel* effectLabel = new QLabel(effectText.trimmed(), item);
    effectLabel->setStyleSheet("font-size: 10px; color: #666;");
    layout->addWidget(effectLabel);

    layout->addStretch();

    QLabel* priceLabel = new QLabel(u8"¥" + QString::number(food.price), item);
    priceLabel->setStyleSheet("font-size: 11px; color: #E91E63; font-weight: bold;");
    priceLabel->setFixedWidth(35);
    layout->addWidget(priceLabel);

    QPushButton* buyBtn = new QPushButton(u8"选择", item);
    buyBtn->setFixedSize(40, 22);
    buyBtn->setProperty("foodId", food.id);
    buyBtn->setProperty("foodPrice", food.price);

    connect(buyBtn, &QPushButton::clicked, this, [this, food]() {
        if (m_feedingInProgress || !m_attr)
            return;
        if (m_attr->getCoin() >= food.price) {
            emit foodSelected(food.id);
            hide();
        }
    });

    m_foodButtons[food.id] = buyBtn;
    layout->addWidget(buyBtn);

    return item;
}

void FoodSelectWidget::setFeedingInProgress(bool busy)
{
    m_feedingInProgress = busy;
    updateCoinDisplay();
}

void FoodSelectWidget::updateCoinDisplay()
{
    if (m_attr && m_coinLabel) {
        int coin = m_attr->getCoin();
        m_coinLabel->setText(u8"金币: " + QString::number(coin));

        PetConfig* config = PetConfig::getInstance();
        QMap<QString, FoodData> foods = config->getFoods();

        for (auto it = foods.begin(); it != foods.end(); ++it) {
            if (m_foodButtons.contains(it.key())) {
                QPushButton* btn = m_foodButtons[it.key()];
                if (m_feedingInProgress) {
                    btn->setEnabled(false);
                    btn->setToolTip(u8"进食中，结束后可再次购买");
                } else {
                    btn->setToolTip({});
                    btn->setEnabled(coin >= it.value().price);
                }
            }
        }
    }
}

void FoodSelectWidget::showAtPos(const QPoint& pos)
{
    updateCoinDisplay();
    move(pos);
    show();
    raise();
    activateWindow();
}

void FoodSelectWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    hide();
}
