#pragma execution_character_set("utf-8")

#include "PetMenuWidget.h"
#include "PetFSM.h"
#include "AttributePanelWidget.h"
#include "FoodSelectWidget.h"
#include "PetController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QScreen>
#include <QApplication>
#include <QPropertyAnimation>

// ==================== SubMenuWidget 实现 ====================

SubMenuWidget::SubMenuWidget(QWidget* parent)
    : QWidget(nullptr)
{
    initStyle();
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(2);
}

void SubMenuWidget::initStyle()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
}

void SubMenuWidget::setItems(const QStringList& items)
{
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    QString btnStyle =
        "QPushButton{"
        "border: 1px solid #aaa;"
        "background: #fff;"
        "font-size: 12px;"
        "padding: 4px 12px;"
        "}"
        "QPushButton:hover{"
        "background: #e0e0e0;"
        "}";

    for (const QString& text : items) {
        QPushButton* btn = new QPushButton(text, this);
        btn->setStyleSheet(btnStyle);
        btn->setFixedHeight(24);
        connect(btn, &QPushButton::clicked, this, [this, text]() {
            if (m_callback) {
                m_callback(text);
            }
            hide();
        });
        m_layout->addWidget(btn);
    }

    adjustSize();
}

void SubMenuWidget::showAtPos(const QPoint& pos)
{
    move(pos);
    show();
    raise();
}

void SubMenuWidget::setCallback(std::function<void(const QString&)> callback)
{
    m_callback = callback;
}

void SubMenuWidget::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    hide();
}

// ==================== PetMenuWidget 实现 ====================

PetMenuWidget::PetMenuWidget(PetFSM* fsm, PetAttribute* attr, PetController* controller, QWidget* parent)
    : QWidget(nullptr), m_petFsm(fsm), m_petAttr(attr), m_controller(controller)
{
    m_attrPanelHideTimer = new QTimer(this);
    m_attrPanelHideTimer->setSingleShot(true);
    m_attrPanelHideTimer->setInterval(100);
    connect(m_attrPanelHideTimer, &QTimer::timeout, this, [this]() {
        if (m_attrPanel) {
            m_attrPanel->hide();
        }
    });

    setupMenuStructure();
    initMenuStyle();
    initMenuUI();
    initConnections();
}

void PetMenuWidget::setupMenuStructure()
{
    m_subMenuItems[u8"喂食"] = QStringList();
    m_subMenuItems[u8"面板"] = QStringList();
    m_subMenuItems[u8"互动"] = QStringList{ u8"学习", u8"玩耍", u8"工作", u8"睡觉" };
    m_subMenuItems[u8"对话"] = QStringList();
    m_subMenuItems[u8"系统"] = QStringList{
        u8"设置",
        u8"更换桌面壁纸",
        u8"导出存档",
        u8"导入存档",
        u8"重置为首次启动",
        u8"退出",
    };
    
    m_attrPanel = new AttributePanelWidget(m_petAttr, this);
    connect(m_attrPanel, &AttributePanelWidget::mouseEntered, this, [this]() {
        m_attrPanelHideTimer->stop();
    });
    connect(m_attrPanel, &AttributePanelWidget::mouseLeft, this, [this]() {
        m_attrPanelHideTimer->start();
    });

    m_foodSelectWidget = new FoodSelectWidget(m_petAttr, this);
    if (m_petFsm) {
        m_foodSelectWidget->setFeedingInProgress(m_petFsm->currentState() == PetStateType::Eat);
        connect(m_petFsm, &PetFSM::stateChanged, this, [this](PetStateType s) {
            if (m_foodSelectWidget) {
                m_foodSelectWidget->setFeedingInProgress(s == PetStateType::Eat);
            }
        });
    }
    connect(m_foodSelectWidget, &FoodSelectWidget::foodSelected, this, [this](const QString& foodId) {
        if (m_controller) {
            m_controller->handleFoodSelection(foodId);
        }
        hide();
        hideSubMenu();
    });
}

void PetMenuWidget::initMenuStyle()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
}

void PetMenuWidget::initMenuUI()
{
    QWidget* mainWidget = new QWidget(this);
    mainWidget->setStyleSheet(
        "QWidget {"
        "background: #fff;"
        "border: 1px solid #aaa;"
        "}"
    );

    m_mainLayout = new QHBoxLayout(mainWidget);
    m_mainLayout->setContentsMargins(4, 4, 4, 4);
    m_mainLayout->setSpacing(3);

    QString btnStyle =
        "QPushButton{"
        "border: 1px solid #aaa;"
        "background: #fff;"
        "font-size: 12px;"
        "padding: 4px 8px;"
        "}"
        "QPushButton:hover{"
        "background: #e0e0e0;"
        "}"
        "QPushButton:pressed{"
        "background: #ccc;"
        "}";

    // 按固定顺序添加菜单项：喂食、面板、互动、对话、系统
    QStringList menuNames = {u8"喂食", u8"面板", u8"互动", u8"对话", u8"系统"};
    for (const QString& name : menuNames) {
        QPushButton* btn = new QPushButton(name, this);
        btn->setStyleSheet(btnStyle);
        btn->setFixedHeight(26);
        btn->setFixedWidth(50);
        m_mainLayout->addWidget(btn);
        m_menuButtons[name] = btn;
    }

    QHBoxLayout* outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(mainWidget);

    adjustSize();

    m_subMenu = new SubMenuWidget(this);
    m_subMenu->setCallback([this](const QString& action) {
        onSubMenuAction(action);
    });
}

void PetMenuWidget::initConnections()
{
    for (auto it = m_menuButtons.begin(); it != m_menuButtons.end(); ++it) {
        const QString& menuName = it.key();
        QPushButton* btn = it.value();

        if (menuName == u8"面板") {
            // 面板按钮：鼠标悬停显示，点击不做任何操作
            connect(btn, &QPushButton::clicked, this, [this, menuName]() {
                // 点击面板按钮不做任何操作，保持点击展开其他子菜单的功能
                // onMenuButtonClicked(menuName);
            });
            // 启用鼠标跟踪
            btn->setMouseTracking(true);
            // 安装事件过滤器来处理鼠标悬停
            btn->installEventFilter(this);
        } else {
            // 其他按钮：点击激活和关闭子菜单
            connect(btn, &QPushButton::clicked, this, [this, menuName]() {
                onMenuButtonClicked(menuName);
            });
            // 禁用鼠标跟踪
            btn->setMouseTracking(false);
        }
    }
}

bool PetMenuWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::HoverEnter) {
        for (auto it = m_menuButtons.begin(); it != m_menuButtons.end(); ++it) {
            if (obj == it.value()) {
                const QString& menuName = it.key();
                if (menuName == u8"面板") {
                    m_attrPanelHideTimer->stop();
                    if (m_attrPanel) {
                        QPoint panelPos = this->pos();
                        panelPos.setY(panelPos.y() - m_attrPanel->height());
                        panelPos.setX(panelPos.x() + (this->width() - m_attrPanel->width()) / 2);
                        m_attrPanel->showAtPos(panelPos);
                    }
                }
                break;
            }
        }
    }
    else if (event->type() == QEvent::HoverLeave) {
        for (auto it = m_menuButtons.begin(); it != m_menuButtons.end(); ++it) {
            if (obj == it.value()) {
                const QString& menuName = it.key();
                if (menuName == u8"面板") {
                    m_attrPanelHideTimer->start();
                }
                break;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PetMenuWidget::showAtPetRect(const QRect& petGlobalRect)
{
    adjustSize();
    const int gap = 6;
    const int w = width();
    const int h = height();
    int x = petGlobalRect.x() + (petGlobalRect.width() - w) / 2;
    int y = petGlobalRect.y() + petGlobalRect.height() + gap;

    QScreen* screen = QApplication::screenAt(QPoint(x + w / 2, y + h / 2));
    if (!screen)
        screen = QApplication::primaryScreen();
    const QRect avail = screen->availableGeometry();

    if (x + w > avail.right())
        x = avail.right() - w + 1;
    if (x < avail.left())
        x = avail.left();
    /* 下方放不下则改到宠物上方 */
    if (y + h > avail.bottom())
        y = petGlobalRect.y() - h - gap;
    if (y < avail.top())
        y = avail.top();

    move(x, y);
    show();
    raise();
    activateWindow();
}

void PetMenuWidget::toggleMenu(const QRect& petGlobalRect)
{
    if (this->isVisible()) {
        hide();
        hideSubMenu();
    }
    else {
        showAtPetRect(petGlobalRect);
    }
}

void PetMenuWidget::refreshFoodsFromConfig()
{
    if (m_foodSelectWidget) {
        m_foodSelectWidget->rebuildFoodListFromConfig();
    }
}

void PetMenuWidget::showSubMenu(const QString& menuName, const QPoint& pos)
{
    // 确保子菜单总是显示在主菜单上方
    QPoint adjustedPos = pos;
    adjustedPos.setY(pos.y() - 2); // 稍微向上偏移一点，避免与主菜单边框重叠
    
    m_subMenu->showAtPos(adjustedPos);
    m_subMenu->setProperty("parentMenu", menuName);
}

void PetMenuWidget::hideSubMenu()
{
    if (m_subMenu) {
        m_subMenu->hide();
    }
}

void PetMenuWidget::toggleSubMenu(const QString& menuName)
{
    QStringList items = m_subMenuItems.value(menuName);
    if (items.isEmpty()) {
        /* 喂食：无二级列表，直接弹出选购；再次点击主栏「喂食」应关闭选购 */
        if (menuName == u8"喂食" && m_foodSelectWidget && m_foodSelectWidget->isVisible()) {
            m_foodSelectWidget->hide();
            return;
        }
        onSubMenuAction(menuName);
        return;
    }

    QPushButton* btn = m_menuButtons.value(menuName);
    if (!btn) return;

    if (m_subMenu->isVisible() && m_subMenu->property("parentMenu").toString() == menuName) {
        hideSubMenu();
        return;
    }

    hideSubMenu();
    
    m_subMenu->setItems(items);
    m_subMenu->adjustSize();
    int subMenuHeight = m_subMenu->height();
    QPoint btnGlobalPos = btn->mapToGlobal(QPoint(0, 0));
    QPoint pos(btnGlobalPos.x(), btnGlobalPos.y() - subMenuHeight);
    showSubMenu(menuName, pos);
}

void PetMenuWidget::onMenuButtonClicked(const QString& menuName)
{
    toggleSubMenu(menuName);
}

void PetMenuWidget::onSubMenuAction(const QString& action)
{
    qDebug() << "[菜单] 选择:" << action;

    if (action == u8"喂食") {
        if (m_foodSelectWidget) {
            QPoint foodPos = this->pos();
            foodPos.setY(foodPos.y() - m_foodSelectWidget->height());
            foodPos.setX(foodPos.x() + (this->width() - m_foodSelectWidget->width()) / 2);
            m_foodSelectWidget->showAtPos(foodPos);
        }
        hideSubMenu();
    }
    else if (action == u8"学习") {
        if (m_controller) m_controller->handleMenuAction(action);
        hide();
        hideSubMenu();
    }
    else if (action == u8"玩耍") {
        if (m_controller) m_controller->handleMenuAction(action);
        hide();
        hideSubMenu();
    }
    else if (action == u8"工作") {
        if (m_controller) m_controller->handleMenuAction(action);
        hide();
        hideSubMenu();
    }
    else if (action == u8"睡觉") {
        if (m_controller) m_controller->handleMenuAction(action);
        hide();
        hideSubMenu();
    }
    else if (action == u8"对话") {
        if (m_controller) {
            m_controller->handleMenuAction(action);
        }
        hide();
        hideSubMenu();
    }
    // 面板不再通过点击触发，而是通过鼠标悬停触发
    // else if (action == u8"面板") {
    //     // 面板逻辑已移到eventFilter中
    // }
    else if (action == u8"设置") {
        emit openSettingsRequested();
        hide();
        hideSubMenu();
    }
    else if (action == u8"更换桌面壁纸") {
        emit changeWallpaperRequested();
        hide();
        hideSubMenu();
    }
    else if (action == u8"导出存档") {
        emit exportSaveRequested();
        hide();
        hideSubMenu();
    }
    else if (action == u8"导入存档") {
        emit importSaveRequested();
        hide();
        hideSubMenu();
    }
    else if (action == u8"重置为首次启动") {
        emit firstLaunchResetRequested();
        hide();
        hideSubMenu();
    }
    else if (action == u8"退出") {
        if (m_controller) m_controller->handleMenuAction(action);
        hide();
        hideSubMenu();
    }
}

void PetMenuWidget::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    // 焦点失去时不再自动关闭菜单
    // hide();
    // hideSubMenu();
}

void PetMenuWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    // 鼠标离开时不再自动关闭菜单
    // hide();
    // hideSubMenu();
}

// 重写鼠标按下事件，实现右键点击关闭菜单
void PetMenuWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        hide();
        hideSubMenu();
    }
    QWidget::mousePressEvent(event);
}
