#pragma execution_character_set("utf-8")

#include "PetWidget.h"
#include "PetMenuWidget.h"
#include "SettingsDialog.h"
#include "PetFSM.h"
#include "PetState.h"
#include "PetConfig.h"
#include "PetController.h"
#include "ChatWidget.h"
#include "PetDatabase.h"
#include "../State/PetStateChat.h"

#include <QMouseEvent>
#include <QMovie>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include <QContextMenuEvent>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QtMath>
#include <QStringList>
#include <QCloseEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPainter>
#include <QPaintEvent>
#if defined(Q_OS_WIN)
#include <Windows.h>
#include <string>
#endif

#if defined(Q_OS_WIN)
static bool setWindowsDesktopWallpaper(const QString& absPath)
{
    const QString native = QDir::toNativeSeparators(absPath);
    const std::wstring w = native.toStdWString();
    return ::SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)w.c_str(),
                                   SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != FALSE;
}
#endif

/** 在固定矩形内对当前帧做等比缩放并居中绘制；避免 QLabel + setScaledContents 拉伸填满时，
 *  因各 GIF 宽高比不同而在切换动画时产生「突然变大/变小」的观感。 */
class ScaledGifWidget : public QWidget
{
public:
    explicit ScaledGifWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet(QStringLiteral("background: transparent;"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    /** 在适配窗口后的像素基础上再乘系数（1=铺满短边方向上的可视区域） */
    void setContentScale(qreal scale)
    {
        const qreal s = qBound(0.05, scale, 4.0);
        if (qAbs(m_contentScale - s) <= 1e-9) {
            return;
        }
        m_contentScale = s;
        update();
    }

    void setMovie(QMovie* movie)
    {
        if (m_movie == movie) {
            return;
        }
        if (m_movie) {
            disconnect(m_movie, nullptr, this, nullptr);
        }
        m_movie = movie;
        if (m_movie) {
            connect(m_movie, &QMovie::frameChanged, this, [this](int) { update(); });
            connect(m_movie, &QMovie::updated, this, [this] { update(); });
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        if (!m_movie) {
            return;
        }
        QPixmap pm = m_movie->currentPixmap();
        if (pm.isNull()) {
            return;
        }
        const QSize box = size();
        const qreal s = qBound(0.05, m_contentScale, 4.0);
        const QSize fitBox(qMax(1, static_cast<int>(box.width() * s)),
                         qMax(1, static_cast<int>(box.height() * s)));
        const QPixmap scaled = pm.scaled(fitBox, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const int x = (box.width() - scaled.width()) / 2;
        const int y = (box.height() - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
    }

private:
    QMovie* m_movie = nullptr;
    qreal m_contentScale = 1.0;
};

// 获取项目根目录
QString getProjectRootPath() {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    // 从 x64/Debug 回溯2级到项目根目录
    dir.cdUp();
    dir.cdUp();
    return dir.absolutePath();
}

// 获取动画绝对路径
QString getAnimationPath(const QString& relativePath) {
    QDir dir(getProjectRootPath());
    return dir.absoluteFilePath("resources/animations/" + relativePath);
}

static QString animationRelPathFromAbsolute(const QString& absolutePath)
{
    const QString marker = QStringLiteral("resources/animations/");
    const int idx = absolutePath.indexOf(marker, 0, Qt::CaseInsensitive);
    if (idx < 0)
        return {};
    QString rel = absolutePath.mid(idx + marker.size());
    return PetConfig::normalizeAnimationRelativePath(rel);
}

// 构造函数
PetWidget::PetWidget(PetFSM* fsm, PetAttribute* attr, QWidget* parent)
    : QWidget(parent), m_petFsm(fsm), m_petAttr(attr)
{
    initWidgtetStyle();
    initGifPlayer();
    initFreeMove();

    // 创建业务控制器和菜单
    m_controller = new PetController(fsm, attr);
    m_petMenu = new PetMenuWidget(fsm, attr, m_controller, this);
    connect(m_petMenu, &PetMenuWidget::firstLaunchResetRequested, this, &PetWidget::resetToFirstLaunchDefaults);
    connect(m_petMenu, &PetMenuWidget::changeWallpaperRequested, this, &PetWidget::onChangeWallpaper);
    connect(m_petMenu, &PetMenuWidget::exportSaveRequested, this, &PetWidget::onExportSave);
    connect(m_petMenu, &PetMenuWidget::importSaveRequested, this, &PetWidget::onImportSave);
    connect(m_petMenu, &PetMenuWidget::openSettingsRequested, this, &PetWidget::onOpenSettings);

    connect(PetConfig::getInstance(), &PetConfig::configReloaded, this, &PetWidget::applyConfigHotReload);

    // 创建聊天窗口
    m_chatWidget = new ChatWidget(this);
    connect(m_chatWidget, &ChatWidget::userInput, this, &PetWidget::onChatUserInput);
    connect(m_chatWidget, &ChatWidget::aiCancelRequested, this, &PetWidget::onAiCancelRequested);
    connect(m_chatWidget, &ChatWidget::closed, this, &PetWidget::onChatClosed);

    // 连接状态机信号
    connect(m_petFsm, &PetFSM::stateChanged, this, &PetWidget::switchStateAnimation);
    connect(m_petFsm, &PetFSM::playAnimation, this, &PetWidget::onPlayAnimation);
    connect(m_petFsm, &PetFSM::showChatWidget, this, &PetWidget::onShowChatWidget);
    connect(m_petFsm, &PetFSM::showChatReply, this, &PetWidget::onChatReply);
    connect(m_petFsm, &PetFSM::chatOllamaPrefetchFailed, this, &PetWidget::onChatOllamaPrefetchFailed);
    connect(m_petFsm, &PetFSM::chatBusyChanged, m_chatWidget, &ChatWidget::setChatGenerating);
    connect(m_petFsm, &PetFSM::chatAssistantStarted, m_chatWidget, &ChatWidget::beginAssistantBubble);
    connect(m_petFsm, &PetFSM::chatAssistantDelta, m_chatWidget, &ChatWidget::appendAssistantBubbleDelta);
    connect(m_petFsm, &PetFSM::chatAssistantFinished, m_chatWidget, &ChatWidget::endAssistantBubble);
    connect(m_petFsm, &PetFSM::stateTransition, this, &PetWidget::saveData);

    // 启动无操作计时器
    resetIdleTimer();

    // 初始化存档系统
    m_savePath = getProjectRootPath() + "/resources/save/save_data.json";
    m_dbPath = getProjectRootPath() + "/resources/save/pet.db";

    // 初始化数据库
    m_database = new PetDatabase(this);
    m_database->initDatabase(m_dbPath);

    // 加载数据（检测并选择数据源）
    loadDataFromSource();

    // 创建自动保存定时器（5分钟）
    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &PetWidget::onAutoSave);
    m_autoSaveTimer->start(5 * 60 * 1000);

    /* 菜单「退出」走 qApp->quit() 时未必触发 closeEvent，此处保证写出 DB + JSON */
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        persistToDatabase();
        flushPortableJson();
    });
}

// 初始化窗口样式
void PetWidget::initWidgtetStyle()
{
    // 获取配置
    PetConfig* config = PetConfig::getInstance();
    int width = config->getWindowWidth();
    int height = config->getWindowHeight();

    // 设置无边框、置顶、工具窗口
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);

    // 设置透明背景
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_AcceptTouchEvents, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    // 设置固定大小
    setFixedSize(width, height);

    // 初始位置：屏幕右下角
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    this->move(screenRect.width() - this->width() - 50, screenRect.height() - this->height() - 50);

    qDebug() << "背景透明:" << testAttribute(Qt::WA_TranslucentBackground);
    qDebug() << "鼠标穿透:" << testAttribute(Qt::WA_TransparentForMouseEvents);
}

// 初始化GIF播放器
void PetWidget::initGifPlayer()
{
    // GIF 绘制区（固定与宠物窗口同大，帧在区内等比居中）
    m_gifLabel = new ScaledGifWidget(this);
    m_gifLabel->setGeometry(0, 0, this->width(), this->height());
    qDebug() << "GIF 区鼠标穿透:" << m_gifLabel->testAttribute(Qt::WA_TransparentForMouseEvents);

    // 加载默认动画
    QString gifPath = getAnimationPath("Idle/Idle.gif");
    if (!QFile::exists(gifPath)) {
        qWarning() << "[GIF错误] 文件不存在:" << gifPath;
        gifPath = "";
    }

    m_gifMovie = new QMovie(gifPath);

    if (!m_gifMovie->isValid()) {
        qWarning() << "[GIF错误] 无效路径或格式:" << gifPath;
        return;
    }

    m_gifLabel->setMovie(m_gifMovie); // ScaledGifWidget：接管绘制，不用 QLabel::setMovie
    applyGifPlayback(gifPath, false, MovieOneShotKind::None);

    qDebug() << "[GIF成功] 已加载:" << gifPath;
}

void PetWidget::disconnectGifOneShotHandlers()
{
    if (!m_gifMovie) return;
    disconnect(m_gifMovie, &QMovie::frameChanged, this, &PetWidget::onGifFrameChanged);
    disconnect(m_gifMovie, &QMovie::updated, this, &PetWidget::onGifUpdatedForOneShot);
    disconnect(m_gifMovie, &QMovie::finished, this, &PetWidget::onMovieOneShotFinished);
}

void PetWidget::applyGifPlayback(const QString& absolutePath, bool playOnce, MovieOneShotKind pendingKind)
{
    if (!m_gifMovie) return;
    disconnectGifOneShotHandlers();

    m_gifMovie->stop();
    m_gifMovie->setFileName(absolutePath);

    if (playOnce) {
        m_movieOneShotKind = pendingKind;
        m_gifOneShotSawLastFrame = false;
        m_gifOneShotSingleFrameHandled = false;
        connect(m_gifMovie, &QMovie::frameChanged, this, &PetWidget::onGifFrameChanged);
        connect(m_gifMovie, &QMovie::updated, this, &PetWidget::onGifUpdatedForOneShot);
    } else {
        m_movieOneShotKind = MovieOneShotKind::None;
    }

    m_gifMovie->start();

    m_currentAnimationRelPath = animationRelPathFromAbsolute(absolutePath);
    if (m_gifLabel) {
        m_gifLabel->setContentScale(PetConfig::getInstance()->animationScaleForRelativePath(m_currentAnimationRelPath));
    }
}

void PetWidget::onGifFrameChanged(int frameNumber)
{
    if (m_movieOneShotKind == MovieOneShotKind::None || !m_gifMovie)
        return;

    const int fc = m_gifMovie->frameCount();
    if (fc <= 1)
        return;

    if (frameNumber == fc - 1) {
        m_gifOneShotSawLastFrame = true;
    } else if (frameNumber == 0 && m_gifOneShotSawLastFrame) {
        disconnectGifOneShotHandlers();
        m_gifMovie->stop();
        onMovieOneShotFinished();
    }
}

void PetWidget::onGifUpdatedForOneShot()
{
    if (m_movieOneShotKind == MovieOneShotKind::None || !m_gifMovie)
        return;
    if (m_gifMovie->frameCount() != 1)
        return;
    if (m_gifOneShotSingleFrameHandled)
        return;
    m_gifOneShotSingleFrameHandled = true;
    disconnectGifOneShotHandlers();
    m_gifMovie->stop();
    onMovieOneShotFinished();
}

void PetWidget::onMovieOneShotFinished()
{
    if (!m_gifMovie) return;
    disconnectGifOneShotHandlers();

    const MovieOneShotKind k = m_movieOneShotKind;
    m_movieOneShotKind = MovieOneShotKind::None;
    m_blockStateAnimationSwitch = false;

    switch (k) {
    case MovieOneShotKind::DoubleClickSpecial:
        switchStateAnimation(m_petFsm->currentState());
        break;
    case MovieOneShotKind::Eat:
        m_petFsm->notifyEatAnimationFinished();
        break;
    case MovieOneShotKind::SleepFallAsleep:
        m_petFsm->notifySleepFallAsleepFinished();
        break;
    default:
        break;
    }
}

// 初始化自由移动
void PetWidget::initFreeMove()
{
    // 获取配置
    PetConfig* config = PetConfig::getInstance();

    // 无操作计时器
    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);
    m_idleTimer->setInterval(config->getIdleTimeout());
    connect(m_idleTimer, &QTimer::timeout, this, &PetWidget::startFreeMove);

    // 移动更新计时器
    m_moveTimer = new QTimer(this);
    m_moveTimer->setInterval(config->getMoveUpdateInterval());
    connect(m_moveTimer, &QTimer::timeout, this, &PetWidget::onFreeMoveUpdate);

    // 初始化随机方向
    m_moveAngle = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
    m_moveDirectionBias = (m_moveAngle > M_PI / 2 && m_moveAngle < 3 * M_PI / 2) ? -1 : 1;
}

// 开始自由移动
void PetWidget::startFreeMove()
{
    // 只有在正常待机状态才自由移动
    if (m_petFsm->currentState() != PetStateType::Idle) {
        return;
    }

    m_isFreeMoving = true;
    m_moveTimer->start();
    updateIdleFreeMoveAnimation();
    qDebug() << "[自由移动] 开始自由移动";
}

// 停止自由移动
void PetWidget::stopFreeMove()
{
    const bool wasMoving = m_isFreeMoving;
    m_isFreeMoving = false;
    m_moveTimer->stop();
    if (wasMoving) {
        m_lastIdleWalkRelPath.clear();
        m_idleWalkAnimSlot = IdleWalkAnimSlot::Stand;
        restoreIdleStandingAnimation();
    }
    qDebug() << "[自由移动] 停止自由移动";
}

void PetWidget::restoreIdleStandingAnimation()
{
    if (!m_gifMovie || m_blockStateAnimationSwitch)
        return;
    if (m_petFsm->currentState() != PetStateType::Idle)
        return;
    const QString abs = getAnimationPath(QStringLiteral("Idle/Idle.gif"));
    if (!QFile::exists(abs))
        return;
    applyGifPlayback(abs, false, MovieOneShotKind::None);
    m_lastIdleWalkRelPath = QStringLiteral("Idle/Idle.gif");
    m_idleWalkAnimSlot = IdleWalkAnimSlot::Stand;
}

void PetWidget::updateIdleFreeMoveAnimation()
{
    if (!m_gifMovie || m_blockStateAnimationSwitch)
        return;
    if (!m_isFreeMoving || m_petFsm->currentState() != PetStateType::Idle)
        return;

    const qreal c = qCos(m_moveAngle);
    /* 移动过程中只播左/右走，不切 Idle，避免「行走↔待机」抖动；
       左右切换用较大滞回阈值，中间方向维持上一帧朝向。 */
    constexpr qreal kFlipSide = 0.30;

    if (m_idleWalkAnimSlot == IdleWalkAnimSlot::Stand) {
        m_idleWalkAnimSlot = (c >= 0) ? IdleWalkAnimSlot::WalkRight : IdleWalkAnimSlot::WalkLeft;
    } else if (m_idleWalkAnimSlot == IdleWalkAnimSlot::WalkLeft) {
        if (c > kFlipSide)
            m_idleWalkAnimSlot = IdleWalkAnimSlot::WalkRight;
    } else {
        if (c < -kFlipSide)
            m_idleWalkAnimSlot = IdleWalkAnimSlot::WalkLeft;
    }

    const QString rel = (m_idleWalkAnimSlot == IdleWalkAnimSlot::WalkLeft)
                            ? QStringLiteral("Move/Move_L.gif")
                            : QStringLiteral("Move/Move_R.gif");

    QString abs = getAnimationPath(rel);
    if (!QFile::exists(abs)) {
        const QString idleRel = QStringLiteral("Idle/Idle.gif");
        abs = getAnimationPath(idleRel);
        if (!QFile::exists(abs))
            return;
        m_idleWalkAnimSlot = IdleWalkAnimSlot::Stand;
        if (idleRel == m_lastIdleWalkRelPath)
            return;
        m_lastIdleWalkRelPath = idleRel;
        applyGifPlayback(abs, false, MovieOneShotKind::None);
        return;
    }

    if (rel == m_lastIdleWalkRelPath)
        return;
    m_lastIdleWalkRelPath = rel;
    applyGifPlayback(abs, false, MovieOneShotKind::None);
}

// 重置无操作计时器
void PetWidget::resetIdleTimer()
{
    // 停止自由移动
    if (m_isFreeMoving) {
        stopFreeMove();
    }

    // 重置无操作计时器
    m_idleTimer->start();
}

// 自由移动更新
void PetWidget::onFreeMoveUpdate()
{
    // 检查是否仍在正常待机状态或正在聊天
    if (m_petFsm->currentState() != PetStateType::Idle || m_isChatting) {
        stopFreeMove();
        return;
    }

    // 获取配置
    PetConfig* config = PetConfig::getInstance();
    qreal speed = config->getMoveSpeed();
    int keepRate = config->getDirectionKeepRate();

    // 获取屏幕边界
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();

    // 获取当前位置
    QPoint currentPos = this->pos();
    int newX = currentPos.x();
    int newY = currentPos.y();

    // 计算移动
    qreal dx = speed * qCos(m_moveAngle);
    qreal dy = speed * qSin(m_moveAngle);

    newX += static_cast<int>(dx);
    newY += static_cast<int>(dy);

    // 边界检测和方向调整
    bool hitBoundary = false;

    // 左边界
    if (newX <= 0) {
        newX = 0;
        hitBoundary = true;
        m_moveAngle = M_PI - m_moveAngle;
        m_moveDirectionBias = 1;
    }
    // 右边界
    else if (newX >= screenRect.width() - this->width()) {
        newX = screenRect.width() - this->width();
        hitBoundary = true;
        m_moveAngle = M_PI - m_moveAngle;
        m_moveDirectionBias = -1;
    }

    // 上边界
    if (newY <= 0) {
        newY = 0;
        hitBoundary = true;
        m_moveAngle = -m_moveAngle;
    }
    // 下边界
    else if (newY >= screenRect.height() - this->height()) {
        newY = screenRect.height() - this->height();
        hitBoundary = true;
        m_moveAngle = -m_moveAngle;
    }

    // 规范化角度到 [0, 2π)
    while (m_moveAngle < 0) m_moveAngle += 2 * M_PI;
    while (m_moveAngle >= 2 * M_PI) m_moveAngle -= 2 * M_PI;

    // 随机微调方向（降低突然反向的概率）
    if (!hitBoundary) {
        // 使用配置的概率保持当前大方向
        if (QRandomGenerator::global()->bounded(100) < keepRate) {
            // 微调角度（±15度）
            qreal adjustment = (QRandomGenerator::global()->generateDouble() - 0.5) * M_PI / 6;
            m_moveAngle += adjustment;

            // 确保保持大方向
            if (m_moveDirectionBias > 0) {
                if (m_moveAngle > M_PI / 2 && m_moveAngle < 3 * M_PI / 2) {
                    m_moveAngle = M_PI - m_moveAngle;
                }
            } else {
                if (m_moveAngle < M_PI / 2 || m_moveAngle > 3 * M_PI / 2) {
                    m_moveAngle = M_PI - m_moveAngle;
                }
            }
        } else {
            // 较大幅度调整（±30度）
            qreal adjustment = (QRandomGenerator::global()->generateDouble() - 0.5) * M_PI / 3;
            m_moveAngle += adjustment;
        }

        // 规范化角度
        while (m_moveAngle < 0) m_moveAngle += 2 * M_PI;
        while (m_moveAngle >= 2 * M_PI) m_moveAngle -= 2 * M_PI;
    }

    // 移动窗口
    this->move(newX, newY);
    updateIdleFreeMoveAnimation();
}

// 鼠标按下事件
void PetWidget::mousePressEvent(QMouseEvent* event)
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[PetWidget] 鼠标按下:" << event->button();
    }

    // 重置无操作计时器
    resetIdleTimer();

    if (event->button() == Qt::LeftButton) {
        // 左键按下：开始拖拽
        m_isDragging = true;
        m_dragStartPos = event->pos();
        // 隐藏菜单
        if (m_petMenu && m_petMenu->isVisible()) {
            m_petMenu->hide();
        }
    }
    else if (event->button() == Qt::RightButton) {
        // 右键按下：切换菜单显示/隐藏
        if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
            qDebug() << "[PetWidget] 右键点击，切换菜单";
        }
        if (m_petMenu) {
            const QRect petGlobal(this->mapToGlobal(QPoint(0, 0)), this->size());
            m_petMenu->toggleMenu(petGlobal);
        }
    }
    QWidget::mousePressEvent(event);
}

// 鼠标移动事件
void PetWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // 计算新位置
        QPoint newPetPos = event->globalPos() - m_dragStartPos;
        // 限制在屏幕范围内
        QScreen* screen = QApplication::primaryScreen();
        QRect screenRect = screen->availableGeometry();
        newPetPos.setX(qBound(0, newPetPos.x(), screenRect.width() - this->width()));
        newPetPos.setY(qBound(0, newPetPos.y(), screenRect.height() - this->height()));

        this->move(newPetPos);
    }
    QWidget::mouseMoveEvent(event);
}

// 鼠标释放事件
void PetWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (PetConfig::getInstance()->isVerboseStateLogsEnabled()) {
        qDebug() << "[PetWidget] 鼠标释放:" << event->button();
    }

    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        // 重置无操作计时器
        resetIdleTimer();
    }
    QWidget::mouseReleaseEvent(event);
}

// 鼠标双击事件
void PetWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        PetConfig* cfg = PetConfig::getInstance();
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const int cd = cfg->getDoubleClickCooldownMs();
        if (cd > 0 && m_lastDoubleClickMs > 0 && (now - m_lastDoubleClickMs) < cd) {
            qDebug() << "[PetWidget] 双击间隔不足，忽略（冷却" << cd << "ms）";
            QWidget::mouseDoubleClickEvent(event);
            return;
        }
        m_lastDoubleClickMs = now;

        resetIdleTimer();

        const PetStateType st = m_petFsm->currentState();
        int moodDelta = 0;
        if (st == PetStateType::Idle) {
            moodDelta = cfg->getDoubleClickMoodDeltaIdle();
        } else if (st == PetStateType::AbnormalIdle) {
            moodDelta = cfg->getDoubleClickMoodDeltaAbnormal();
        }

        const int h0 = m_petAttr->getHunger();
        const int e0 = m_petAttr->getEnergy();
        const int m0 = m_petAttr->getMood();
        const int exp0 = m_petAttr->getExp();
        const int coin0 = m_petAttr->getCoin();

        if (moodDelta != 0) {
            m_petAttr->changeMood(moodDelta);
        }

        const QString scene = (st == PetStateType::Idle)
                                  ? QStringLiteral("正常待机")
                                  : (st == PetStateType::AbnormalIdle ? QStringLiteral("异常待机") : QStringLiteral("其它"));
        QStringList deltas;
        deltas << PetState::formatSettlementAttrDelta(QStringLiteral("饱食"), 0);
        deltas << PetState::formatSettlementAttrDelta(QStringLiteral("精力"), 0);
        deltas << PetState::formatSettlementAttrDelta(QStringLiteral("心情"), moodDelta);
        qDebug() << "[结算]" << QStringLiteral("双击互动") << "|" << scene << "|" << deltas.join(QStringLiteral(" "))
                 << "| 结算前" << h0 << e0 << m0 << exp0 << coin0 << QStringLiteral("→ 结算后")
                 << m_petAttr->getHunger() << m_petAttr->getEnergy() << m_petAttr->getMood()
                 << m_petAttr->getExp() << m_petAttr->getCoin() << QStringLiteral("| 升级：") << 0;

        m_petFsm->onDoubleClick();
    }
    QWidget::mouseDoubleClickEvent(event);
}

// 右键菜单事件（忽略）
void PetWidget::contextMenuEvent(QContextMenuEvent* event)
{
    event->ignore();
}

// 切换状态动画
void PetWidget::resetToFirstLaunchDefaults()
{
    const auto ret = QMessageBox::question(this,
                                           QStringLiteral("恢复为首次使用？"),
                                           QStringLiteral(
                                               "宠物会回到「第一次养它」时的状态：等级、经验、饱食、精力、心情和金币都会用设置里的默认数值，"
                                               "之后都会按这个新的起点继续养。\n\n"
                                               "和宠物聊过的内容、以及系统记下的对话摘要将全部清空，聊天窗口也会变回刚打开时的样子。\n\n"
                                               "此操作无法撤销，确定要继续吗？"),
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    if (m_petFsm->currentState() == PetStateType::Chat) {
        PetStateChat* chatState = dynamic_cast<PetStateChat*>(m_petFsm->currentStateObject());
        if (chatState) {
            chatState->wipeChatTurnsSummaryAndSave();
        }
    } else {
        PetStateChat::writeEmptyChatMemoryFile();
    }
    if (m_chatWidget) {
        m_chatWidget->resetConversationView();
    }

    m_petAttr->resetToConfigDefaults();
    persistToDatabase();
    flushPortableJson();

    if (m_petFsm->currentState() != PetStateType::Idle) {
        m_petFsm->changeState(PetStateType::Idle);
    }
    if (m_chatWidget) {
        m_chatWidget->hide();
    }
    m_isChatting = false;
}

void PetWidget::switchStateAnimation(PetStateType state)
{
    if (!m_gifMovie) return;

    if (m_blockStateAnimationSwitch) {
        return;
    }

    // 非正常待机状态时停止自由移动
    if (state != PetStateType::Idle && m_isFreeMoving) {
        stopFreeMove();
    }

    QString relativePath;
    switch (state) {
    case PetStateType::Idle:
        relativePath = QStringLiteral("Idle/Idle.gif");
        resetIdleTimer();
        break;
    case PetStateType::AbnormalIdle:
        relativePath = QStringLiteral("AbnormalIdle/AbnormalIdle.gif");
        break;
    case PetStateType::Eat:
        relativePath = QStringLiteral("Eat/Eat.gif");
        break;
    case PetStateType::Sleep:
        return;  // 睡眠状态由状态类自己控制动画
    case PetStateType::Chat:
        return;  // 聊天动画由 PetStateChat::enter 中 requestPlayAnimation 控制
    case PetStateType::Play:
        relativePath = QStringLiteral("Play/Play.gif");
        break;
    case PetStateType::Study:
        relativePath = QStringLiteral("Study/Study.gif");
        break;
    case PetStateType::Work:
        relativePath = QStringLiteral("Work/Work.gif");
        break;
    default:
        relativePath = QStringLiteral("Idle/Idle.gif");
        break;
    }

    const QString gifPath = getAnimationPath(relativePath);
    if (!QFile::exists(gifPath)) {
        qWarning() << "[状态动画错误] 文件不存在:" << gifPath;
        return;
    }

    if (state == PetStateType::Eat) {
        m_blockStateAnimationSwitch = true;
        applyGifPlayback(gifPath, true, MovieOneShotKind::Eat);
        qDebug() << "[状态动画] 进食（单次）:" << gifPath;
        return;
    }

    applyGifPlayback(gifPath, false, MovieOneShotKind::None);
    qDebug() << "[状态动画] 切换到状态" << static_cast<int>(state) << "路径:" << gifPath;
}

// 播放动画
void PetWidget::onPlayAnimation(const QString& animationPath, bool isStateAnimation, bool playOnce)
{
    if (!m_gifMovie) return;

    qDebug() << "[播放动画]" << animationPath << "状态动画:" << isStateAnimation << "单次:" << playOnce;

    const QString gifPath = getAnimationPath(animationPath);
    if (!QFile::exists(gifPath)) {
        qWarning() << "[动画错误] 文件不存在:" << gifPath;
        return;
    }

    if (!isStateAnimation && playOnce) {
        m_blockStateAnimationSwitch = true;
        applyGifPlayback(gifPath, true, MovieOneShotKind::DoubleClickSpecial);
        return;
    }

    if (isStateAnimation && playOnce) {
        m_blockStateAnimationSwitch = true;
        applyGifPlayback(gifPath, true, MovieOneShotKind::SleepFallAsleep);
        return;
    }

    applyGifPlayback(gifPath, false, MovieOneShotKind::None);
}

// 聊天相关槽函数
void PetWidget::onShowChatWidget()
{
    if (m_chatWidget) {
        // 显示在宠物窗口的右侧
        QPoint petPos = this->pos();
        QSize petSize = this->size();
        QSize chatSize = m_chatWidget->size();
        
        QPoint chatPos(petPos.x() + petSize.width() + 10, 
                       petPos.y() + (petSize.height() - chatSize.height()) / 2);
        
        // 确保聊天窗口在屏幕内
        QScreen* screen = QApplication::primaryScreen();
        QRect screenRect = screen->availableGeometry();
        if (chatPos.x() + chatSize.width() > screenRect.width()) {
            chatPos.setX(petPos.x() - chatSize.width() - 10);
        }
        if (chatPos.y() < 0) {
            chatPos.setY(0);
        } else if (chatPos.y() + chatSize.height() > screenRect.height()) {
            chatPos.setY(screenRect.height() - chatSize.height());
        }
        
        m_chatWidget->showAtPos(chatPos);
        m_isChatting = true;
    }
}

void PetWidget::onChatReply(const QString& reply)
{
    if (m_chatWidget) {
        m_chatWidget->addChatMessage("宠物", reply, false);
    }
}

void PetWidget::onChatClosed()
{
    m_isChatting = false;
    if (m_petFsm->currentState() == PetStateType::Chat) {
        // 退出聊天状态，回到空闲状态
        m_petFsm->changeState(PetStateType::Idle);
    }
}

void PetWidget::onChatUserInput(const QString& input)
{
    if (m_petFsm->currentState() == PetStateType::Chat) {
        PetState* currentState = m_petFsm->currentStateObject();
        PetStateChat* chatState = dynamic_cast<PetStateChat*>(currentState);
        if (chatState) {
            chatState->onUserInput(input);
        }
    }
}

void PetWidget::onChatOllamaPrefetchFailed(const QString& message)
{
    QMessageBox::warning(this,
                         QStringLiteral("无法连接本地 Ollama"),
                         message.trimmed());
}

void PetWidget::onAiCancelRequested()
{
    if (m_petFsm->currentState() != PetStateType::Chat) {
        return;
    }
    PetStateChat* chatState = dynamic_cast<PetStateChat*>(m_petFsm->currentStateObject());
    if (chatState) {
        chatState->cancelCurrentChat();
    }
}

void PetWidget::onAutoSave()
{
    persistToDatabase();
}

void PetWidget::saveData()
{
    persistToDatabase();
}

void PetWidget::persistToDatabase()
{
    m_database->saveAttribute(m_petAttr);
}

void PetWidget::flushPortableJson()
{
    m_petAttr->saveToFile(m_savePath);
}

void PetWidget::onChangeWallpaper()
{
#if !defined(Q_OS_WIN)
    QMessageBox::information(this,
                             QStringLiteral("提示"),
                             QStringLiteral("当前平台不支持更换 Windows 桌面壁纸。"));
    return;
#else
    const QString path = QFileDialog::getOpenFileName(this,
                                                      QStringLiteral("选择壁纸图片"),
                                                      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                      QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("失败"), QStringLiteral("文件不存在。"));
        return;
    }
    if (!setWindowsDesktopWallpaper(path)) {
        QMessageBox::warning(this,
                             QStringLiteral("失败"),
                             QStringLiteral("无法将该图片设为桌面壁纸，可尝试更换格式或路径。"));
    }
#endif
}

void PetWidget::onExportSave()
{
    const QString defName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + QStringLiteral("/save_data.json");
    const QString path = QFileDialog::getSaveFileName(this,
                                                      QStringLiteral("导出存档"),
                                                      defName,
                                                      QStringLiteral("JSON 存档 (*.json);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    if (!m_petAttr->saveToFile(path)) {
        QMessageBox::warning(this,
                             QStringLiteral("失败"),
                             QStringLiteral("导出失败，请检查路径是否可写。"));
        return;
    }
    flushPortableJson();
}

void PetWidget::onImportSave()
{
    const QString path = QFileDialog::getOpenFileName(this,
                                                      QStringLiteral("导入存档"),
                                                      QString(),
                                                      QStringLiteral("JSON 存档 (*.json);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    if (!m_petAttr->loadFromFile(path)) {
        QMessageBox::warning(this,
                             QStringLiteral("失败"),
                             QStringLiteral("无法读取该文件，格式可能不正确。"));
        return;
    }
    persistToDatabase();
    flushPortableJson();
}

void PetWidget::onOpenSettings()
{
    SettingsDialog dlg(this);
    connect(&dlg, &SettingsDialog::chatMemoryEdited, this, [this]() {
        if (PetStateChat* cs = dynamic_cast<PetStateChat*>(m_petFsm->stateObject(PetStateType::Chat))) {
            cs->reloadChatMemoryFromDisk();
        }
    });
    dlg.exec();
}

void PetWidget::applyConfigHotReload()
{
    PetConfig* config = PetConfig::getInstance();
    const int w = config->getWindowWidth();
    const int h = config->getWindowHeight();
    setFixedSize(w, h);
    if (m_gifLabel) {
        m_gifLabel->setGeometry(0, 0, w, h);
    }
    if (m_idleTimer) {
        m_idleTimer->setInterval(config->getIdleTimeout());
    }
    if (m_moveTimer) {
        m_moveTimer->setInterval(config->getMoveUpdateInterval());
    }
    if (m_gifLabel && !m_currentAnimationRelPath.isEmpty()) {
        m_gifLabel->setContentScale(config->animationScaleForRelativePath(m_currentAnimationRelPath));
    }
    resetIdleTimer();
    if (m_petMenu) {
        m_petMenu->refreshFoodsFromConfig();
    }
}

bool PetWidget::loadDataFromSource()
{
    const bool fileExists = QFile::exists(m_savePath);
    const bool dbExists = m_database->databaseExists();

    if (!fileExists && !dbExists) {
        qDebug() << "[存档] 无存档，使用默认属性";
        return true;
    }

    /* 仅一侧存在：直接载入 */
    if (fileExists && !dbExists) {
        if (!m_petAttr->loadFromFile(m_savePath)) {
            qWarning() << "[存档] JSON 读取失败";
            return false;
        }
        m_database->saveAttribute(m_petAttr);
        qDebug() << "[存档] 仅有 JSON，已载入并同步到数据库";
        return true;
    }

    if (!fileExists && dbExists) {
        if (!m_database->loadAttribute(m_petAttr)) {
            return false;
        }
        m_petAttr->notifyAttributeChanged();
        qDebug() << "[存档] 从数据库载入（当前无 JSON 存档文件）";
        return true;
    }

    /* JSON 与数据库同时存在：比对；不一致则询问（异常退出时 JSON 往往落后于 DB） */
    PetAttribute tempAttr;
    PetAttribute tempAttrDb;
    const bool okFile = tempAttr.loadFromFile(m_savePath);
    const bool okDb = m_database->loadAttribute(&tempAttrDb);

    if (!okFile && !okDb) {
        qWarning() << "[存档] JSON 与数据库均无法读取";
        return false;
    }
    if (!okFile) {
        if (!m_database->loadAttribute(m_petAttr)) {
            return false;
        }
        m_petAttr->notifyAttributeChanged();
        qDebug() << "[存档] JSON 无效，从数据库载入";
        return true;
    }
    if (!okDb) {
        if (!m_petAttr->loadFromFile(m_savePath)) {
            return false;
        }
        m_database->saveAttribute(m_petAttr);
        qDebug() << "[存档] 数据库无效，从 JSON 载入";
        return true;
    }

    const bool same = (
        tempAttr.getLevel() == tempAttrDb.getLevel() &&
        tempAttr.getExp() == tempAttrDb.getExp() &&
        tempAttr.getHunger() == tempAttrDb.getHunger() &&
        tempAttr.getEnergy() == tempAttrDb.getEnergy() &&
        tempAttr.getMood() == tempAttrDb.getMood() &&
        tempAttr.getCoin() == tempAttrDb.getCoin()
    );

    if (same) {
        if (!m_petAttr->loadFromFile(m_savePath)) {
            return false;
        }
        m_database->saveAttribute(m_petAttr);
        qDebug() << "[存档] JSON 与数据库一致，从 JSON 载入";
        return true;
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle(QStringLiteral("数据源冲突"));
    msgBox.setText(QStringLiteral(
        "检测到 JSON 存档与数据库中的属性不一致。\n"
        "若上次为异常退出，JSON 可能尚未更新，数据库通常更接近真实进度。\n\n"
        "请在本次启动中选择使用哪一份数据？"));
    msgBox.setIcon(QMessageBox::Question);

    QPushButton* fileButton = msgBox.addButton(QStringLiteral("使用 JSON 文件"), QMessageBox::AcceptRole);
    QPushButton* dbButton = msgBox.addButton(QStringLiteral("使用数据库"), QMessageBox::AcceptRole);
    QPushButton* cancelButton = msgBox.addButton(QMessageBox::Cancel);

    msgBox.exec();

    if (msgBox.clickedButton() == fileButton) {
        if (!m_petAttr->loadFromFile(m_savePath)) {
            return false;
        }
        m_database->saveAttribute(m_petAttr);
        qDebug() << "[存档] 用户选择 JSON";
        return true;
    }
    if (msgBox.clickedButton() == dbButton) {
        if (!m_database->loadAttribute(m_petAttr)) {
            return false;
        }
        flushPortableJson();
        m_petAttr->notifyAttributeChanged();
        qDebug() << "[存档] 用户选择数据库，已用当前进度覆盖 JSON";
        return true;
    }

    Q_UNUSED(cancelButton);
    qDebug() << "[存档] 用户取消选择，使用默认属性";
    return true;
}

void PetWidget::closeEvent(QCloseEvent* event)
{
    persistToDatabase();
    flushPortableJson();
    QWidget::closeEvent(event);
}
