#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QPoint>
#include <QLabel>
#include <QMovie>
#include <QTimer>  // 新增：必须包含QTimer头文件

// 前置声明
class PetFSM;
class PetAttribute;
class PetMenuWidget;
enum class PetStateType; // 状态枚举

class PetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PetWidget(PetFSM* fsm, PetAttribute* attr, QWidget* parent = nullptr);

protected:
    // 重写鼠标事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    // 重写右键菜单事件（屏蔽系统菜单）
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    // 状态切换时切换动画
    void switchStateAnimation(PetStateType state);

private:
    // 初始化窗口样式
    void initWidgtetStyle();
    // 初始化GIF播放器
    void initGifPlayer();

private:
    // 核心依赖
    PetFSM* m_petFsm = nullptr;          // 状态机
    PetAttribute* m_petAttr = nullptr;   // 宠物属性
    PetMenuWidget* m_petMenu = nullptr;  // 右键菜单

    // GIF播放相关
    QLabel* m_gifLabel = nullptr;        // 承载GIF的标签
    QMovie* m_gifMovie = nullptr;        // GIF播放对象

    // 拖拽相关
    bool m_isDragging = false;           // 是否正在拖拽
    QPoint m_dragStartPos;               // 拖拽起始位置（鼠标相对窗口）

    QTimer* m_stateTimer;
};