#pragma execution_character_set("utf-8")

#include "ChatWidget.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QScrollBar>
#include <QSizePolicy>
#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

namespace {

/* 气泡最大宽度固定；用户与宠物共用同一上限（可按需改数值） */
constexpr int kChatBubbleMaxWidth = 360;

QWidget* makeBubbleWrap(QWidget* rowParent, QLabel* contentLabel)
{
    QWidget* wrap = new QWidget(rowParent);
    auto* vl = new QVBoxLayout(wrap);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);
    vl->addWidget(contentLabel);
    wrap->setMinimumWidth(0);
    wrap->setMaximumWidth(kChatBubbleMaxWidth);
    contentLabel->setMinimumWidth(0);
    contentLabel->setMaximumWidth(kChatBubbleMaxWidth);
    return wrap;
}

} // namespace

ChatWidget::ChatWidget(QWidget* parent)
    : QWidget(parent)
{
    initStyle();
    initUI();
    initConnections();
}

void ChatWidget::initStyle()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setMinimumSize(320, 260);
    resize(480, 380);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(255, 255, 255));
    setPalette(palette);
    setAutoFillBackground(true);

    setStyleSheet(
        "ChatWidget {"
        "    background-color: #ffffff;"
        "    border: 1px solid #aaa;"
        "    border-radius: 8px;"
        "}"
    );
}

void ChatWidget::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 头部区域
    m_headerWidget = new QWidget(this);
    m_headerWidget->setStyleSheet(
        "background-color: #f0f0f0;"
        "border-top-left-radius: 8px;"
        "border-top-right-radius: 8px;"
        "border-bottom: 1px solid #ddd;"
    );
    m_headerWidget->setFixedHeight(30);

    QHBoxLayout* headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(8, 0, 8, 0);
    headerLayout->setSpacing(0);

    m_titleLabel = new QLabel("宠物对话", this);
    m_titleLabel->setStyleSheet("font-size: 12px; color: #333;");
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch();

    m_closeButton = new QPushButton(this);
    m_closeButton->setText("×");
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 14px;"
        "    color: #666;"
        "    background: transparent;"
        "    border: none;"
        "    width: 24px;"
        "    height: 24px;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "    color: #333;"
        "}"
    );
    m_closeButton->setFixedSize(24, 24);
    headerLayout->addWidget(m_closeButton);

    m_mainLayout->addWidget(m_headerWidget);

    // 聊天内容区域
    m_chatScrollArea = new QScrollArea(this);
    m_chatScrollArea->setWidgetResizable(true);
    m_chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatScrollArea->setStyleSheet(
        "QScrollArea {"
        "    border: none;"
        "    background-color: #ffffff;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "    background-color: #ffffff;"
        "}"
        "QScrollBar:vertical {"
        "    width: 8px;"
        "    background: #f5f5f5;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(0, 0, 0, 0.25);"
        "    border-radius: 4px;"
        "}"
    );

    m_chatContentWidget = new QWidget(this);
    m_chatContentWidget->setStyleSheet(QStringLiteral("background-color: #ffffff;"));
    m_chatLayout = new QVBoxLayout(m_chatContentWidget);
    m_chatLayout->setContentsMargins(12, 12, 12, 12);
    m_chatLayout->setSpacing(8);

    // 添加欢迎消息
    addChatMessage("宠物", "你好呀！有什么想和我聊的吗？", false);

    m_chatScrollArea->setWidget(m_chatContentWidget);
    m_mainLayout->addWidget(m_chatScrollArea, 1);

    // 输入区域
    QWidget* inputWidget = new QWidget(this);
    inputWidget->setStyleSheet("background-color: #f8f8f8; border-top: 1px solid #ddd;");
    inputWidget->setFixedHeight(50);

    QHBoxLayout* inputLayout = new QHBoxLayout(inputWidget);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(8);

    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setPlaceholderText("输入内容...（回车发送，最大200字）");
    m_inputEdit->setStyleSheet(
        "QLineEdit {"
        "    font-size: 12px;"
        "    padding: 6px 8px;"
        "    border: 1px solid #ddd;"
        "    border-radius: 16px;"
        "    background: white;"
        "}"
    );
    m_sendButton = new QPushButton(QStringLiteral("发送"), this);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 12px;"
        "    color: white;"
        "    background-color: #4a90e2;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    border-radius: 16px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #357abd;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #b0bec5;"
        "}"
    );

    m_cancelButton = new QPushButton(QStringLiteral("取消"), this);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 12px;"
        "    color: #444;"
        "    background-color: #ececec;"
        "    border: 1px solid #ccc;"
        "    padding: 6px 12px;"
        "    border-radius: 16px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ddd;"
        "}"
    );
    m_cancelButton->hide();

    inputLayout->addWidget(m_inputEdit, 1);
    inputLayout->addWidget(m_cancelButton);
    inputLayout->addWidget(m_sendButton);

    m_mainLayout->addWidget(inputWidget);
}

void ChatWidget::initConnections()
{
    connect(m_sendButton, &QPushButton::clicked, this, &ChatWidget::onSendButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ChatWidget::onCancelButtonClicked);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &ChatWidget::onInputReturnPressed);
    connect(m_closeButton, &QPushButton::clicked, this, &ChatWidget::onCloseButtonClicked);
}

void ChatWidget::showAtPos(const QPoint& pos)
{
    move(pos);
    show();
    raise();
    activateWindow();
    m_inputEdit->setFocus();
}

void ChatWidget::resetConversationView()
{
    setChatGenerating(false);
    m_streamContentLabel = nullptr;
    QLayoutItem* item = nullptr;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    addChatMessage(QStringLiteral("宠物"), QStringLiteral("你好呀！有什么想和我聊的吗？"), false);
}

void ChatWidget::scrollChatToBottom()
{
    QMetaObject::invokeMethod(this, [this]() {
        QScrollBar* scrollBar = m_chatScrollArea->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }, Qt::QueuedConnection);
}

void ChatWidget::setChatGenerating(bool generating)
{
    m_isGenerating = generating;
    m_inputEdit->setReadOnly(generating);
    m_sendButton->setEnabled(!generating);
    m_cancelButton->setVisible(generating);
}

void ChatWidget::beginAssistantBubble()
{
    if (m_streamContentLabel) {
        return;
    }

    QWidget* messageWidget = new QWidget(this);
    QHBoxLayout* messageLayout = new QHBoxLayout(messageWidget);
    messageLayout->setContentsMargins(0, 0, 0, 0);
    messageLayout->setSpacing(8);
    messageLayout->setAlignment(Qt::AlignTop);

    QLabel* senderLabel = new QLabel(QStringLiteral("宠物:"), messageWidget);
    senderLabel->setStyleSheet("font-size: 11px; color: #666; font-weight: bold;");
    senderLabel->setFixedWidth(50);
    senderLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QLabel* contentLabel = new QLabel(QStringLiteral("······"), messageWidget);
    contentLabel->setStyleSheet(
        QStringLiteral(
            "font-size: 12px; color: #333;"
            "background: #f5f5f5;"
            "padding: 8px 12px;"
            "border-radius: 12px;")
    );
    contentLabel->setWordWrap(true);
    contentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    contentLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QWidget* bubbleWrap = makeBubbleWrap(messageWidget, contentLabel);

    messageLayout->addWidget(senderLabel, 0, Qt::AlignTop);
    messageLayout->addWidget(bubbleWrap, 0, Qt::AlignTop);
    messageLayout->addStretch(1);

    m_chatLayout->addWidget(messageWidget);
    m_streamContentLabel = contentLabel;

    scrollChatToBottom();
}

void ChatWidget::appendAssistantBubbleDelta(const QString& text)
{
    if (!m_streamContentLabel || text.isEmpty()) {
        return;
    }
    QString cur = m_streamContentLabel->text();
    const QString idleHint = QStringLiteral("······");
    if (cur == idleHint) {
        cur.clear();
    }
    m_streamContentLabel->setText(cur + text);
    scrollChatToBottom();
}

void ChatWidget::endAssistantBubble(bool success, bool userCancelled, const QString& errorMessage)
{
    if (!m_streamContentLabel) {
        return;
    }
    QString cur = m_streamContentLabel->text().trimmed();
    const QString idleHint = QStringLiteral("······");
    if (cur == idleHint) {
        cur.clear();
    }

    if (success && cur.isEmpty()) {
        m_streamContentLabel->setText(QStringLiteral("（暂时没有收到文字回复）"));
        m_streamContentLabel = nullptr;
        scrollChatToBottom();
        return;
    }

    if (userCancelled) {
        if (cur.isEmpty()) {
            m_streamContentLabel->setText(QStringLiteral("（生成已取消）"));
        } else {
            m_streamContentLabel->setText(cur + QStringLiteral("\n（生成已取消）"));
        }
    } else if (!success && !errorMessage.trimmed().isEmpty()) {
        if (cur.isEmpty()) {
            m_streamContentLabel->setText(errorMessage.trimmed());
        } else {
            m_streamContentLabel->setText(cur + QLatin1Char('\n') + errorMessage.trimmed());
        }
    }

    m_streamContentLabel = nullptr;
    scrollChatToBottom();
}

void ChatWidget::addChatMessage(const QString& sender, const QString& message, bool isUser)
{
    QWidget* messageWidget = new QWidget(this);
    QHBoxLayout* messageLayout = new QHBoxLayout(messageWidget);
    messageLayout->setContentsMargins(0, 0, 0, 0);
    messageLayout->setSpacing(8);
    messageLayout->setAlignment(Qt::AlignTop);

    QLabel* senderLabel = new QLabel(sender + ":", messageWidget);
    senderLabel->setStyleSheet("font-size: 11px; color: #666; font-weight: bold;");
    senderLabel->setFixedWidth(50);
    senderLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QLabel* contentLabel = new QLabel(message, messageWidget);
    QString backgroundColor = isUser ? "#e3f2fd" : "#f5f5f5";
    contentLabel->setStyleSheet(
        "font-size: 12px; color: #333;"
        "background: " + backgroundColor + ";"
        "padding: 8px 12px;"
        "border-radius: 12px;"
    );
    contentLabel->setWordWrap(true);
    contentLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    contentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QWidget* bubbleWrap = makeBubbleWrap(messageWidget, contentLabel);

    if (isUser) {
        messageLayout->addStretch(1);
        messageLayout->addWidget(bubbleWrap, 0, Qt::AlignTop);
        messageLayout->addWidget(senderLabel, 0, Qt::AlignTop);
    } else {
        messageLayout->addWidget(senderLabel, 0, Qt::AlignTop);
        messageLayout->addWidget(bubbleWrap, 0, Qt::AlignTop);
        messageLayout->addStretch(1);
    }

    m_chatLayout->addWidget(messageWidget);

    scrollChatToBottom();
}

void ChatWidget::onSendButtonClicked()
{
    if (m_isGenerating) {
        return;
    }
    QString input = m_inputEdit->text().trimmed();
    if (!input.isEmpty()) {
        addChatMessage("你", input, true);
        emit userInput(input);
        m_inputEdit->clear();
    }
}

void ChatWidget::onInputReturnPressed()
{
    onSendButtonClicked();
}

void ChatWidget::onCancelButtonClicked()
{
    emit aiCancelRequested();
}

void ChatWidget::onCloseButtonClicked()
{
    hide();
    emit closed();
}

void ChatWidget::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    // 不自动关闭，让用户手动关闭
}

void ChatWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < 30) {
        m_isDragging = true;
        m_dragStartPos = event->globalPos() - this->frameGeometry().topLeft();
    }
}

void ChatWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging) {
        move(event->globalPos() - m_dragStartPos);
    }
}

void ChatWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

#if defined(Q_OS_WIN)
bool ChatWidget::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST) {
            const int sx = static_cast<int>(static_cast<short>(LOWORD(msg->lParam)));
            const int sy = static_cast<int>(static_cast<short>(HIWORD(msg->lParam)));
            const QPoint client = mapFromGlobal(QPoint(sx, sy));
            const int m = 8;
            const int w = width();
            const int h = height();
            if (w >= m * 2 && h >= m * 2) {
                const bool left = client.x() < m;
                const bool right = client.x() > w - m;
                const bool top = client.y() < m;
                const bool bottom = client.y() > h - m;
                if (top && left) {
                    *result = HTTOPLEFT;
                    return true;
                }
                if (top && right) {
                    *result = HTTOPRIGHT;
                    return true;
                }
                if (bottom && left) {
                    *result = HTBOTTOMLEFT;
                    return true;
                }
                if (bottom && right) {
                    *result = HTBOTTOMRIGHT;
                    return true;
                }
                if (left) {
                    *result = HTLEFT;
                    return true;
                }
                if (right) {
                    *result = HTRIGHT;
                    return true;
                }
                if (top) {
                    *result = HTTOP;
                    return true;
                }
                if (bottom) {
                    *result = HTBOTTOM;
                    return true;
                }
            }
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}
#endif
