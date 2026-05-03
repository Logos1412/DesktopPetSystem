#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QLabel>
#include <QMovie>
#include <QTimer>
#include <QPoint>
#include "PetFSM.h"

class PetAttribute;
class PetMenuWidget;
class ChatWidget;
class PetDatabase;
class PetController;

// 宠物主窗口类
class PetWidget : public QWidget
{
    Q_OBJECT

public:
    PetWidget(PetFSM* fsm, PetAttribute* attr, QWidget* parent = nullptr);

    // 开始自由移动
    void startFreeMove();
    // 停止自由移动
    void stopFreeMove();
    // 重置无操作计时器
    void resetIdleTimer();

private:
    // 初始化窗口样式
    void initWidgtetStyle();
    // 初始化GIF播放器
    void initGifPlayer();
    // 初始化自由移动
    void initFreeMove();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

public slots:
    // 聊天相关槽函数
    void onShowChatWidget();
    void onChatReply(const QString& reply);
    void onChatClosed();
    void onChatUserInput(const QString& input);
    void onAiCancelRequested();
    void onChatOllamaPrefetchFailed(const QString& message);
    // 存档保存
    void saveData();
    bool loadDataFromSource();
    void syncDataToBoth();

signals:
    // 聊天用户输入信号
    void chatUserInput(const QString& input);

private slots:
    void resetToFirstLaunchDefaults();
    void switchStateAnimation(PetStateType state);
    void onPlayAnimation(const QString& animationPath, bool isStateAnimation = true);
    void onAnimationFinished();
    void onFreeMoveUpdate();
    void onAutoSave();

private:
    PetFSM* m_petFsm = nullptr;             // 状态机指针
    PetAttribute* m_petAttr = nullptr;      // 属性指针
    PetMenuWidget* m_petMenu = nullptr;     // 菜单指针
    PetController* m_controller = nullptr;  // 业务控制器

    QLabel* m_gifLabel = nullptr;           // GIF标签
    QMovie* m_gifMovie = nullptr;           // GIF动画对象

    bool m_isDragging = false;              // 是否正在拖拽
    QPoint m_dragStartPos;                  // 拖拽起始位置
    bool m_isPlayingSpecialAnimation = false;   // 是否正在播放特殊动画

    // 自由移动相关
    QTimer* m_idleTimer = nullptr;          // 无操作计时器
    QTimer* m_moveTimer = nullptr;          // 移动更新计时器
    bool m_isFreeMoving = false;            // 是否正在自由移动
    qreal m_moveAngle = 0.0;                // 移动方向角度（弧度）
    int m_moveDirectionBias = 0;            // 方向偏向（用于保持大方向移动）

    // 聊天相关
    ChatWidget* m_chatWidget = nullptr;     // 聊天界面指针
    bool m_isChatting = false;              // 是否正在聊天

    // 存档相关
    QTimer* m_autoSaveTimer = nullptr;      // 自动保存定时器
    QString m_savePath;                     // 存档路径
    PetDatabase* m_database = nullptr;      // 数据库管理器
    QString m_dbPath;                       // 数据库路径
};
