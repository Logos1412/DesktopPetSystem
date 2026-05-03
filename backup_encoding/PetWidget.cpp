#pragma execution_character_set("utf-8")

#include "PetWidget.h"
#include "PetMenuWidget.h"  // �����˵��ؼ�ͷ�ļ�
#include "PetFSM.h"         // ����״̬��ͷ�ļ�

#include <QMouseEvent>
#include <QMovie>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include <QContextMenuEvent>
#include <QFile>

PetWidget::PetWidget(PetFSM* fsm, PetAttribute* attr, QWidget* parent)
    : QWidget(parent), m_petFsm(fsm), m_petAttr(attr)
{
    // ��ʼ�����ﴰ��
    initWidgtetStyle();
    initGifPlayer();

    // �����˵�
    m_petMenu = new PetMenuWidget(fsm, attr, this);
    // ������־��ȷ�ϲ˵������ɹ�
    qDebug() << "�˵������Ƿ�Ϊ�գ�" << (m_petMenu == nullptr);

    // ��״̬�л��ź� �� �л�����
    connect(m_petFsm, &PetFSM::stateChanged, this, &PetWidget::switchStateAnimation);

    m_stateTimer = new QTimer(this);
    connect(m_stateTimer, &QTimer::timeout, m_petFsm, &PetFSM::onStateUpdate);
    m_stateTimer->start(1000);
}

// ��ʼ��������ʽ
void PetWidget::initWidgtetStyle()
{
    // �ޱ߿� �ö� ���ߴ��ڣ���������ͼ�꣩
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);

    // ����͸��
    setAttribute(Qt::WA_TranslucentBackground);
    // ȷ�����ڽ�������¼�
    setAttribute(Qt::WA_AcceptTouchEvents, true);
    // ������괩͸���ؼ��������ղ�������¼���
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    // ���ڴ�С
    setFixedSize(200, 200);

    // ��ʼλ�� ��Ļ���½�
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    this->move(screenRect.width() - this->width() - 50, screenRect.height() - this->height() - 50);

    // ������־����֤��������
    qDebug() << "����͸�����ԣ�" << testAttribute(Qt::WA_TranslucentBackground);
    qDebug() << "������괩͸���ԣ�" << testAttribute(Qt::WA_TransparentForMouseEvents);
}

// ��ʼ��������ǩ
void PetWidget::initGifPlayer()
{
    // 1. ��������GIF�ı�ǩ���ؼ������ñ�ǩ����͸����
    m_gifLabel = new QLabel(this);
    m_gifLabel->setGeometry(0, 0, this->width(), this->height()); // ��������
    m_gifLabel->setScaledContents(true); // GIF����Ӧ���ڴ�С
    m_gifLabel->setStyleSheet("background: transparent;"); // ��ǩ����͸��

    // ���ģ�����QLabel������¼����գ��¼���͸��������PetWidget
    m_gifLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    qDebug() << "QLabel��괩͸���ԣ�" << m_gifLabel->testAttribute(Qt::WA_TransparentForMouseEvents);

    // 2. ����͸��GIF������ʹ�þ���·�����ԣ����������·����
    QString gifPath = "../resources/animations/idle/idle.gif";
    // ����������ļ��Ƿ����
    if (!QFile::exists(gifPath)) {
        qWarning() << "[GIF·������] �ļ������ڣ�" << gifPath;
        qWarning() << "������ʱʹ�þ���·�������磺C:/pet/animations/idle/idle.gif";
        // ���ף�ʹ�ÿ�GIF������������
        gifPath = "";
    }

    m_gifMovie = new QMovie(gifPath);

    // 3. ��֤GIF��Ч��
    if (!m_gifMovie->isValid()) {
        qWarning() << "[GIF���Ŵ���] ·����Ч���ʽ��֧�֣�" << gifPath;
        qWarning() << "���飺1.·���Ƿ���ȷ 2.GIF�Ƿ�Ϊ͸����ʽ 3.�ļ�δ��";
        return;
    }

    // 4. ��GIF����ǩ������
    m_gifLabel->setMovie(m_gifMovie);
    m_gifMovie->start(); // ����ѭ������

    // 5. ��ӡ�ɹ���־
    qDebug() << "[GIF���ųɹ�] ͸��GIF������ɣ�·����" << gifPath;
}

// ������� ��ק��ʼ��
void PetWidget::mousePressEvent(QMouseEvent* event)
{
    qDebug() << "[PetWidget] �յ���갴���¼���" << event->button();

    if (event->button() == Qt::LeftButton) {		// ���������ק
        m_isDragging = true;
        m_dragStartPos = event->pos();				// ��¼�����Դ���λ��
        if (m_petMenu && m_petMenu->isVisible()) {	// ���ز˵�
            m_petMenu->hide();
        }
    }
    // �Ҽ�������ʾ�Զ���˵�
    else if (event->button() == Qt::RightButton) {
        qDebug() << "[PetWidget] �Ҽ����£���ʾ�˵�";
        if (m_petMenu) {
            m_petMenu->showAtPos(event->globalPos());
        }
    }
    QWidget::mousePressEvent(event);
}

// ����ƶ�  ��ק
void PetWidget::mouseMoveEvent(QMouseEvent* event)
{
    // �޸�����buttons()�ж�����Ƿ�ס��move�¼���button()ʼ�շ���NoButton��
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        qDebug() << "[PetWidget] ִ����ק�߼�";

        // ���������λ�ã�ȫ������ - �����Դ���λ�ã�
        QPoint newPetPos = event->globalPos() - m_dragStartPos;
        // ��Ļ�߽���
        QScreen* screen = QApplication::primaryScreen();
        QRect screenRect = screen->availableGeometry();
        // ����X/Y������Ļ��
        newPetPos.setX(qBound(0, newPetPos.x(), screenRect.width() - this->width()));
        newPetPos.setY(qBound(0, newPetPos.y(), screenRect.height() - this->height()));
        // �ƶ�����
        this->move(newPetPos);
    }
    QWidget::mouseMoveEvent(event);
}

// ����ͷ� ��ק����
void PetWidget::mouseReleaseEvent(QMouseEvent* event)
{
    qDebug() << "[PetWidget] �յ�����ͷ��¼���" << event->button();

    // ����ͷţ�������ק
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

// ����ϵͳĬ���Ҽ��˵�
void PetWidget::contextMenuEvent(QContextMenuEvent* event)
{
    // ����ϵͳ�˵���ȷ���Զ���˵���Ч
    event->ignore();
}

// ״̬�л����Ŷ�Ӧ����
void PetWidget::switchStateAnimation(PetStateType state) {
    if (!m_gifMovie) return; // �������ж�

    QString gifPath;
    // ��״̬���ö�ӦGIF·��
    switch (state) {
    case PetStateType::Idle:
        gifPath = "../resources/animations/idle/idle.gif";
        break;
    case PetStateType::AbnormalIdle:
        gifPath = "../resources/animations/AbnormalIdle/AbnormalIdle.gif";
        break;
    case PetStateType::Eat:
        gifPath = "../resources/animations/eat/eat.gif";
        break;
    case PetStateType::Sleep:
        gifPath = "../resources/animations/sleep/sleep.gif";
        break;
    case PetStateType::Play:
        gifPath = "../resources/animations/play/play.gif";
        break;
    case PetStateType::Study:
        gifPath = "../resources/animations/study/study.gif";
        break;
    case PetStateType::Work:
        gifPath = "../resources/animations/work/work.gif";
        break;
    default:
        gifPath = "../resources/animations/idle/idle.gif";
        break;
    }

    // ����ļ��Ƿ����
    if (!QFile::exists(gifPath)) {
        qWarning() << "[״̬��������] GIF�ļ������ڣ�" << gifPath;
        return;
    }

    // �л�����
    m_gifMovie->stop();
    m_gifMovie->setFileName(gifPath);
    m_gifMovie->start();
    qDebug() << "[״̬�����л�] �л�����" << static_cast<int>(state) << "��GIF·����" << gifPath;
}