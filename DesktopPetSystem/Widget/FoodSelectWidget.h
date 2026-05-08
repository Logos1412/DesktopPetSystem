#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include "PetConfig.h"

class PetAttribute;

class FoodSelectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FoodSelectWidget(PetAttribute* attr, QWidget* parent = nullptr);

    void showAtPos(const QPoint& pos);
    /** pet_config 热更新后重建食物列表 */
    void rebuildFoodListFromConfig();
    /** 进食结算与 5s 展示期间为 true：购买按钮置灰 */
    void setFeedingInProgress(bool busy);

signals:
    void foodSelected(const QString& foodId);

protected:
    void leaveEvent(QEvent* event) override;

private:
    void initStyle();
    void initUI();
    void updateCoinDisplay();
    QWidget* createFoodItem(const FoodData& food);

    PetAttribute* m_attr = nullptr;
    QLabel* m_coinLabel = nullptr;
    QVBoxLayout* m_foodLayout = nullptr;
    QMap<QString, QPushButton*> m_foodButtons;
    bool m_feedingInProgress = false;
};
