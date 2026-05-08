#pragma execution_character_set("utf-8")

#include "FoodSelectWidget.h"
#include "PetAttribute.h"
#include "PetConfig.h"

#include <algorithm>

#include <QStringList>
#include <QVector>
#include <QFrame>
#include <QSizePolicy>

namespace {

QVector<FoodData> foodsSortedByPriceAscending(const QMap<QString, FoodData>& foods)
{
    QVector<FoodData> v;
    v.reserve(foods.size());
    for (auto it = foods.begin(); it != foods.end(); ++it)
        v.append(it.value());
    std::sort(v.begin(), v.end(), [](const FoodData& a, const FoodData& b) {
        if (a.price != b.price)
            return a.price < b.price;
        return a.name < b.name;
    });
    return v;
}

} // namespace

FoodSelectWidget::FoodSelectWidget(PetAttribute* attr, QWidget* parent)
    : QWidget(nullptr), m_attr(attr)
{
    initStyle();
    initUI();
}

void FoodSelectWidget::initStyle()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::Popup);
    setMinimumWidth(300);
    setMaximumWidth(420);
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
    const QVector<FoodData> foods = foodsSortedByPriceAscending(config->getFoods());

    for (const FoodData& fd : foods) {
        QWidget* item = createFoodItem(fd);
        m_foodLayout->addWidget(item);
    }

    foodContainer->setLayout(m_foodLayout);
    mainLayout->addWidget(foodContainer);

    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background: #ddd;");
    mainLayout->addWidget(line2);

    m_coinLabel = new QLabel(u8"金币: 0", this);
    m_coinLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #FF9800;");
    m_coinLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_coinLabel);

    updateCoinDisplay();
    adjustSize();
}

void FoodSelectWidget::rebuildFoodListFromConfig()
{
    if (!m_foodLayout)
        return;

    QLayoutItem* item = nullptr;
    while ((item = m_foodLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    m_foodButtons.clear();

    PetConfig* config = PetConfig::getInstance();
    const QVector<FoodData> foods = foodsSortedByPriceAscending(config->getFoods());
    for (const FoodData& fd : foods) {
        m_foodLayout->addWidget(createFoodItem(fd));
    }
    updateCoinDisplay();
    adjustSize();
}

QWidget* FoodSelectWidget::createFoodItem(const FoodData& food)
{
    QWidget* item = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(item);
    layout->setContentsMargins(2, 4, 2, 4);
    layout->setSpacing(8);

    auto* textBlock = new QWidget(item);
    auto* textLay = new QVBoxLayout(textBlock);
    textLay->setContentsMargins(0, 0, 0, 0);
    textLay->setSpacing(2);

    QLabel* nameLabel = new QLabel(food.name, textBlock);
    nameLabel->setWordWrap(true);
    nameLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 12px;"));
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QStringList effectParts;
    if (food.hunger > 0)
        effectParts << (u8"饱食+" + QString::number(food.hunger));
    if (food.energy > 0)
        effectParts << (u8"精力+" + QString::number(food.energy));
    if (food.mood > 0)
        effectParts << (u8"心情+" + QString::number(food.mood));
    if (food.exp > 0)
        effectParts << (u8"经验+" + QString::number(food.exp));

    QLabel* effectLabel = new QLabel(effectParts.isEmpty() ? QString() : effectParts.join(QStringLiteral(" ")), textBlock);
    effectLabel->setWordWrap(true);
    effectLabel->setStyleSheet(QStringLiteral("font-size: 10px; color: #666;"));
    effectLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    textLay->addWidget(nameLabel);
    textLay->addWidget(effectLabel);

    textBlock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addWidget(textBlock, 1);

    auto* rightCol = new QWidget(item);
    auto* rightLay = new QVBoxLayout(rightCol);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(4);
    rightLay->setAlignment(Qt::AlignTop);

    QLabel* priceLabel = new QLabel(u8"¥" + QString::number(food.price), rightCol);
    priceLabel->setStyleSheet("font-size: 11px; color: #E91E63; font-weight: bold;");
    priceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    priceLabel->setMinimumWidth(42);

    QPushButton* buyBtn = new QPushButton(u8"选择", rightCol);
    buyBtn->setFixedSize(48, 26);
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
    rightLay->addWidget(priceLabel);
    rightLay->addWidget(buyBtn);
    rightCol->setFixedWidth(62);

    layout->addWidget(rightCol, 0, Qt::AlignTop);

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
