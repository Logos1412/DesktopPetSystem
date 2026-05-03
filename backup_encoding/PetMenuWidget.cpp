#pragma execution_character_set("utf-8")

#include "PetMenuWidget.h"
#include "PetStateSleep.h"
#include "PetStateEat.h"
// 包含其他具体状态头文件
#include "PetStatePlay.h"
#include "PetStateStudy.h"
#include "PetStateWork.h"

#include <QVBoxLayout>
#include <QScreen>
#include <QApplication>
#include <QDebug>
// �����������˵�����ͷ�ļ�
#include <QMenu>
#include <QPushButton>
#include <QWidgetAction>

PetMenuWidget::PetMenuWidget(PetFSM* fsm, PetAttribute* attr, QWidget* parent)
    : QWidget(parent), m_petFsm(fsm), m_petAttr(attr)
{
    // ��ʼ��
    initMenuStyle();
    initMenuButtons();
    initMenuLayout();
    bindButtonClicked();

    // ��ʼ����
    this->hide();
}

// �˵���ʽ�������޸ģ�
void PetMenuWidget::initMenuStyle()
{
    // �ޱ߿� �ö� ���ߴ���(��������ͼ��)
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint | Qt::Tool);

    // ����͸��
    setAttribute(Qt::WA_TranslucentBackground);

    // �̶��˵���С�����������Ӳ˵�������Ϊ100��
    setFixedSize(100, 220);
}

// ��ť��ʽ�����컥����ťΪ�����˵���
void PetMenuWidget::initMenuButtons()
{
    // ����ԭ�а�ť
    m_feedBtn = new QPushButton("ιʳ", this);
    // ���컥����ťΪ������ʽ
    m_interactBtn = new QPushButton("����", this);
    m_wakeUpBtn = new QPushButton("����", this);
    m_exitBtn = new QPushButton("�˳�", this);

    // ���ð�ť��С��������ȵ�����
    QSize btnSize(90, 40);
    m_feedBtn->setFixedSize(btnSize);
    m_interactBtn->setFixedSize(btnSize);
    m_wakeUpBtn->setFixedSize(btnSize);
    m_exitBtn->setFixedSize(btnSize);

    // Բ�� ������ɫ����ʽ���䣩
    QString btnStyle =
        "QPushButton{"
        "border-radius: 8px; /* Բ�� */"
        "border: 1px solid #666; /* �߿� */"
        "background: #f5f5f5; /* ����ɫ */"
        "font-size: 14px; /* �����С */"
        "}"
        "QPushButton:hover{"
        "background: #e0e0e0; /* ���������ɫ */"
        "}";

    // ��ťͳһ��ʽ
    m_feedBtn->setStyleSheet(btnStyle);
    m_interactBtn->setStyleSheet(btnStyle);
    m_wakeUpBtn->setStyleSheet(btnStyle);
    m_exitBtn->setStyleSheet(btnStyle);

    // ������Ϊ������ť���������Ӳ˵�
    m_interactMenu = new QMenu(this);
    // �����Ӳ˵���
    m_interactMenu->addAction("˯��");
    m_interactMenu->addAction("��ˣ");
    m_interactMenu->addAction("ѧϰ");
    m_interactMenu->addAction("����");
    // ���û�����ť�������˵�
    m_interactBtn->setMenu(m_interactMenu);
}

// ���ֳ�ʼ���������޸ģ�
void PetMenuWidget::initMenuLayout()
{
    // ������ֱ����
    QVBoxLayout* vLayout = new QVBoxLayout(this);

    // ��� �߾�
    vLayout->setSpacing(10);
    vLayout->setContentsMargins(5, 5, 5, 5);

    // ���ӵ�������
    vLayout->addWidget(m_feedBtn);
    vLayout->addWidget(m_interactBtn);
    vLayout->addWidget(m_wakeUpBtn);
    vLayout->addWidget(m_exitBtn);

    // ������Ч
    this->setLayout(vLayout);
}

// ����߼������ĸ��죺������ť���Ӳ˵���
// 按钮逻辑绑定（新增：互动按钮有子菜单）
void PetMenuWidget::bindButtonClicked()
{
    // 投喂：切换到进食状态
    connect(m_feedBtn, &QPushButton::clicked, this, [&]() {
        qDebug() << "[菜单]点击投喂->进入进食状态";
        
        PetState* state = m_petFsm->getState(PetStateType::Eat);
        PetStateEat* eatState = dynamic_cast<PetStateEat*>(state);
        if (eatState) {
            eatState->setFoodBonus(30, 5, 5);
        }
        
        m_petFsm->switchState(PetStateType::Eat);
        this->hide();
        });

    // ���������������˵�����߼�
    connect(m_interactMenu, &QMenu::triggered, this, [&](QAction* action) {
        QString actionText = action->text();
        qDebug() << "[�˵�]�������-" << actionText;

        // ���ݲ˵����л���Ӧ״̬
        if (actionText == "˯��") {
            m_petFsm->switchState(PetStateType::Sleep);
        }
        else if (actionText == "��ˣ") {
            m_petFsm->switchState(PetStateType::Play);
        }
        else if (actionText == "ѧϰ") {
            m_petFsm->switchState(PetStateType::Study);
        }
        else if (actionText == "����") {
            m_petFsm->switchState(PetStateType::Work);
        }

        this->hide(); // ��������ز˵�
        });

    // ���ѣ�ԭ���߼����䣩
    connect(m_wakeUpBtn, &QPushButton::clicked, this, [&]() {
        if (m_petFsm->currentState() == PetStateType::Sleep) {
            // ��ȫת��Ϊ����״̬�����û��ѷ���
            PetState* state = m_petFsm->getState(PetStateType::Sleep);
            PetStateSleep* sleepState = dynamic_cast<PetStateSleep*>(state);
            if (sleepState) {
                qDebug() << "[�˵�] ������� �� ��������߻���";
                sleepState->wakeUp();
            }
        }
        else {
            qDebug() << "[�˵�] ������� �� ����δ���ߣ����軽��";
        }
        this->hide();
        });

    // �˳���ԭ���߼����䣩
    connect(m_exitBtn, &QPushButton::clicked, this, [&]() {
        qDebug() << "[�˵�] ����˳� �� �ر�����������";
        QApplication::quit(); // Qt��׼�˳���ʽ���Զ�����������Դ
        });
}

// ��ʾ�˵��������޸ģ�
void PetMenuWidget::showAtPos(const QPoint& globalPos)
{
    // 1. ����˵���ʾλ�ã�����Ҽ�λ�õ�**�Ҳ�10px**�������Y����루������ѣ�
    QPoint menuPos = globalPos + QPoint(10, 0);

    // 2. �߽��⣺��ȡ��Ļ���������ų���������
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();

    // X���⣺����˵��Ҳ೬����Ļ������ʾ�����**���10px**
    if (menuPos.x() + this->width() > screenRect.width()) {
        menuPos = globalPos - QPoint(this->width() + 10, 0);
    }
    // Y���⣺����˵��ײ�������Ļ����������Ļ�ײ�
    if (menuPos.y() + this->height() > screenRect.height()) {
        menuPos.setY(screenRect.height() - this->height() - 10);
    }
    // Y���⣺����˵�����������Ļ����������Ļ����
    if (menuPos.y() < 0) {
        menuPos.setY(10);
    }

    // 3. �ƶ��˵���������λ�ã�Ȼ����ʾ
    this->move(menuPos);
    this->show(); // ��ʾ�˵�����
}