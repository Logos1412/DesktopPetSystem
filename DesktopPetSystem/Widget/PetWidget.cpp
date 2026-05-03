#pragma execution_character_set("utf-8")

#include "PetWidget.h"
#include "PetMenuWidget.h"
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
#include <QCloseEvent>

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
    // 创建GIF标签
    m_gifLabel = new QLabel(this);
    m_gifLabel->setGeometry(0, 0, this->width(), this->height());
    m_gifLabel->setScaledContents(true);
    m_gifLabel->setStyleSheet("background: transparent;");

    // 设置标签鼠标穿透
    m_gifLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    qDebug() << "QLabel鼠标穿透:" << m_gifLabel->testAttribute(Qt::WA_TransparentForMouseEvents);

    // 加载默认动画
    QString gifPath = getAnimationPath("idle/idle.gif");
    if (!QFile::exists(gifPath)) {
        qWarning() << "[GIF错误] 文件不存在:" << gifPath;
        gifPath = "";
    }

    m_gifMovie = new QMovie(gifPath);

    if (!m_gifMovie->isValid()) {
        qWarning() << "[GIF错误] 无效路径或格式:" << gifPath;
        return;
    }

    m_gifLabel->setMovie(m_gifMovie);
    m_gifMovie->start();

    qDebug() << "[GIF成功] 已加载:" << gifPath;
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
    qDebug() << "[自由移动] 开始自由移动";
}

// 停止自由移动
void PetWidget::stopFreeMove()
{
    m_isFreeMoving = false;
    m_moveTimer->stop();
    qDebug() << "[自由移动] 停止自由移动";
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
            m_petMenu->toggleMenu(this->pos());
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
        qDebug() << "[PetWidget] 鼠标双击";
        // 重置无操作计时器
        resetIdleTimer();
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
    syncDataToBoth();

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

    // 如果正在播放特殊动画，不切换
    if (m_isPlayingSpecialAnimation) {
        return;
    }

    // 非正常待机状态时停止自由移动
    if (state != PetStateType::Idle && m_isFreeMoving) {
        stopFreeMove();
    }

    QString gifPath;
    switch (state) {
    case PetStateType::Idle:
        gifPath = getAnimationPath("idle/idle.gif");
        // 切换到正常待机时重置无操作计时器
        resetIdleTimer();
        break;
    case PetStateType::AbnormalIdle:
        gifPath = getAnimationPath("AbnormalIdle/AbnormalIdle.gif");
        break;
    case PetStateType::Eat:
        gifPath = getAnimationPath("eat/eat.gif");
        break;
    case PetStateType::Sleep:
        return;  // 睡眠状态由状态类自己控制动画
    case PetStateType::Play:
        gifPath = getAnimationPath("play/play.gif");
        break;
    case PetStateType::Study:
        gifPath = getAnimationPath("study/study.gif");
        break;
    case PetStateType::Work:
        gifPath = getAnimationPath("work/work.gif");
        break;
    default:
        gifPath = getAnimationPath("idle/idle.gif");
        break;
    }

    if (!QFile::exists(gifPath)) {
        qWarning() << "[状态动画错误] 文件不存在:" << gifPath;
        return;
    }

    m_gifMovie->stop();
    m_gifMovie->setFileName(gifPath);
    m_gifMovie->start();
    qDebug() << "[状态动画] 切换到状态" << static_cast<int>(state) << "路径:" << gifPath;
}

// 播放动画
void PetWidget::onPlayAnimation(const QString& animationPath, bool isStateAnimation)
{
    if (!m_gifMovie) return;

    qDebug() << "[播放动画]" << animationPath << "是否状态动画:" << isStateAnimation;

    QString gifPath = getAnimationPath(animationPath);
    if (!QFile::exists(gifPath)) {
        qWarning() << "[动画错误] 文件不存在:" << gifPath;
        return;
    }

    m_gifMovie->stop();
    m_gifMovie->setFileName(gifPath);
    m_gifMovie->start();

    // 如果不是状态动画，使用配置的时长后恢复
    if (!isStateAnimation) {
        m_isPlayingSpecialAnimation = true;
        int duration = PetConfig::getInstance()->getSpecialAnimationDuration();
        QTimer::singleShot(duration, this, &PetWidget::onAnimationFinished);
    }
}

// 动画播放完成
void PetWidget::onAnimationFinished()
{
    qDebug() << "[动画完成] 恢复状态动画";
    m_isPlayingSpecialAnimation = false;
    switchStateAnimation(m_petFsm->currentState());
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
    syncDataToBoth();
}

void PetWidget::saveData()
{
    syncDataToBoth();
}

void PetWidget::syncDataToBoth()
{
    m_petAttr->saveToFile(m_savePath);
    m_database->saveAttribute(m_petAttr);
}

bool PetWidget::loadDataFromSource()
{
    bool fileExists = QFile::exists(m_savePath);
    bool dbExists = m_database->databaseExists();

    if (!fileExists && !dbExists) {
        qDebug() << "[存档] 两个数据源都不存在，使用默认属性";
        return true;
    }

    if (fileExists && !dbExists) {
        qDebug() << "[存档] 只有文件存在，从文件加载";
        return m_petAttr->loadFromFile(m_savePath);
    }

    if (!fileExists && dbExists) {
        qDebug() << "[存档] 只有数据库存在，从数据库加载";
        return m_database->loadAttribute(m_petAttr);
    }

    PetAttribute tempAttr;
    if (!tempAttr.loadFromFile(m_savePath)) {
        qDebug() << "[存档] 文件加载失败，从数据库加载";
        return m_database->loadAttribute(m_petAttr);
    }

    PetAttribute tempAttrDb;
    if (!m_database->loadAttribute(&tempAttrDb)) {
        qDebug() << "[存档] 数据库加载失败，从文件加载";
        return m_petAttr->loadFromFile(m_savePath);
    }

    bool isSame = (
        tempAttr.getLevel() == tempAttrDb.getLevel() &&
        tempAttr.getExp() == tempAttrDb.getExp() &&
        tempAttr.getHunger() == tempAttrDb.getHunger() &&
        tempAttr.getEnergy() == tempAttrDb.getEnergy() &&
        tempAttr.getMood() == tempAttrDb.getMood() &&
        tempAttr.getCoin() == tempAttrDb.getCoin()
    );

    if (isSame) {
        qDebug() << "[存档] 两个数据源一致，从文件加载";
        return m_petAttr->loadFromFile(m_savePath);
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle("数据源冲突");
    msgBox.setText("检测到文件和数据库中的宠物属性不一致，请问使用哪个数据源？");
    msgBox.setIcon(QMessageBox::Question);

    QPushButton* fileButton = msgBox.addButton("使用文件数据", QMessageBox::AcceptRole);
    QPushButton* dbButton = msgBox.addButton("使用数据库数据", QMessageBox::AcceptRole);
    QPushButton* cancelButton = msgBox.addButton(QMessageBox::Cancel);

    msgBox.exec();

    if (msgBox.clickedButton() == fileButton) {
        qDebug() << "[存档] 用户选择从文件加载";
        bool result = m_petAttr->loadFromFile(m_savePath);
        if (result) {
            m_database->saveAttribute(m_petAttr);
        }
        return result;
    } else if (msgBox.clickedButton() == dbButton) {
        qDebug() << "[存档] 用户选择从数据库加载";
        bool result = m_database->loadAttribute(m_petAttr);
        if (result) {
            m_petAttr->saveToFile(m_savePath);
        }
        return result;
    } else {
        qDebug() << "[存档] 用户取消选择，使用默认属性";
        return true;
    }
}

void PetWidget::closeEvent(QCloseEvent* event)
{
    syncDataToBoth();
    QWidget::closeEvent(event);
}
