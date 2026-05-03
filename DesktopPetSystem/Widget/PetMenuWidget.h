#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QFocusEvent>
#include <QMap>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <functional>
#include "PetFSM.h"

class PetAttribute;
class QPushButton;
class QPropertyAnimation;
class AttributePanelWidget;
class FoodSelectWidget;
class PetController;

class SubMenuWidget : public QWidget
{
    Q_OBJECT

public:
    SubMenuWidget(QWidget* parent = nullptr);
    void setItems(const QStringList& items);
    void showAtPos(const QPoint& pos);
    void setCallback(std::function<void(const QString&)> callback);

protected:
    void focusOutEvent(QFocusEvent* event) override;

private:
    void initStyle();
    QVBoxLayout* m_layout = nullptr;
    std::function<void(const QString&)> m_callback;
};

class PetMenuWidget : public QWidget
{
    Q_OBJECT

public:
    PetMenuWidget(PetFSM* fsm, PetAttribute* attr, PetController* controller, QWidget* parent = nullptr);
    void showAtPos(const QPoint& petPos);
    void toggleMenu(const QPoint& petPos);

signals:
    /** 用户选择将属性恢复为配置文件默认值（等同首次启动存档） */
    void firstLaunchResetRequested();

private:
    void initMenuStyle();
    void initMenuUI();
    void initConnections();
    void setupMenuStructure();
    void showSubMenu(const QString& menuName, const QPoint& pos);
    void hideSubMenu();
    void toggleSubMenu(const QString& menuName);

private slots:
    void onMenuButtonClicked(const QString& menuName);
    void onSubMenuAction(const QString& action);

protected:
    void focusOutEvent(QFocusEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    PetFSM* m_petFsm = nullptr;
    PetAttribute* m_petAttr = nullptr;
    PetController* m_controller = nullptr;
    QHBoxLayout* m_mainLayout = nullptr;
    SubMenuWidget* m_subMenu = nullptr;
    AttributePanelWidget* m_attrPanel = nullptr;
    FoodSelectWidget* m_foodSelectWidget = nullptr;
    QMap<QString, QPushButton*> m_menuButtons;
    QMap<QString, QStringList> m_subMenuItems;
    QTimer* m_attrPanelHideTimer = nullptr;
};
