#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QMovie>
#include <QTimer>
#include <QPoint>
#include <QtGlobal>
#include "PetFSM.h"

class PetAttribute;
class ScaledGifWidget;
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

signals:
    // 聊天用户输入信号
    void chatUserInput(const QString& input);

private slots:
    void resetToFirstLaunchDefaults();
    void onChangeWallpaper();
    void onExportSave();
    void onImportSave();
    void onOpenSettings();
    void applyConfigHotReload();
    void switchStateAnimation(PetStateType state);
    void onPlayAnimation(const QString& animationPath, bool isStateAnimation = true, bool playOnce = false);
    void onMovieOneShotFinished();
    void onGifFrameChanged(int frameNumber);
    void onGifUpdatedForOneShot();
    void onFreeMoveUpdate();
    void onAutoSave();

private:
    PetFSM* m_petFsm = nullptr;             // 状态机指针
    PetAttribute* m_petAttr = nullptr;      // 属性指针
    PetMenuWidget* m_petMenu = nullptr;     // 菜单指针
    PetController* m_controller = nullptr;  // 业务控制器

    ScaledGifWidget* m_gifLabel = nullptr;  // GIF：固定窗口内等比居中，避免切换素材时比例乱跳
    QMovie* m_gifMovie = nullptr;           // GIF动画对象
    /** 当前 GIF 相对路径（resources/animations/），用于缩放热更新 */
    QString m_currentAnimationRelPath;

    enum class MovieOneShotKind {
        None,
        DoubleClickSpecial,
        Eat,
        SleepFallAsleep
    };

    /** 随机移动：行走左/右 + 停下的待机；移动中不切 Idle，靠滞回减闪烁 */
    enum class IdleWalkAnimSlot { Stand, WalkLeft, WalkRight };

    void applyGifPlayback(const QString& absolutePath, bool playOnce, MovieOneShotKind pendingKind);
    void disconnectGifOneShotHandlers();
    /** 桌面随机移动时按水平方向切换左/行走动画；停下时恢复站立待机 */
    void updateIdleFreeMoveAnimation();
    void restoreIdleStandingAnimation();

    bool m_isDragging = false;              // 是否正在拖拽
    QPoint m_dragStartPos;                  // 拖拽起始位置
    qint64 m_lastDoubleClickMs = 0;         // 上次有效双击时间（毫秒时间戳）
    /** 单次 GIF（双击特殊 / 进食 / 入睡过渡）播放期间，阻止 switchStateAnimation 覆盖 */
    bool m_blockStateAnimationSwitch = false;
    MovieOneShotKind m_movieOneShotKind = MovieOneShotKind::None;
    /** 多帧 GIF：已显示最后一帧，等待下一帧回到 0 视为播完一轮 */
    bool m_gifOneShotSawLastFrame = false;
    /** 单帧 GIF：首轮绘制后结束 */
    bool m_gifOneShotSingleFrameHandled = false;

    // 自由移动相关
    QTimer* m_idleTimer = nullptr;          // 无操作计时器
    QTimer* m_moveTimer = nullptr;          // 移动更新计时器
    bool m_isFreeMoving = false;            // 是否正在自由移动
    qreal m_moveAngle = 0.0;                // 移动方向角度（弧度）
    int m_moveDirectionBias = 0;            // 方向偏向（用于保持大方向移动）
    /** 当前播放的待机行走相对路径，避免每帧重复 setMovie */
    QString m_lastIdleWalkRelPath;
    IdleWalkAnimSlot m_idleWalkAnimSlot = IdleWalkAnimSlot::Stand;

    // 聊天相关
    ChatWidget* m_chatWidget = nullptr;     // 聊天界面指针
    bool m_isChatting = false;              // 是否正在聊天

    // 存档相关
    QTimer* m_autoSaveTimer = nullptr;      // 自动保存定时器
    QString m_savePath;                     // 存档路径
    PetDatabase* m_database = nullptr;      // 数据库管理器
    QString m_dbPath;                       // 数据库路径

    /** 运行中持久化：仅 SQLite */
    void persistToDatabase();
    /** 可移动存档：写入 save_data.json（退出或显式导出前调用） */
    void flushPortableJson();
};
