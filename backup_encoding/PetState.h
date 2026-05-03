#pragma once
#pragma execution_character_set("utf-8")

#include <qstring.h>
#include <qtimer.h>
#include <qobject.h>
#include "PetAttribute.h"

// ǰ������������ѭ������
class PetFSM;

// ����״̬����ö��
enum class PetStateType {
	Idle,				//	正常
	AbnormalIdle,	// 异常正常
	Eat,				//	进食
	Sleep,				//	睡觉
	Play,				//	玩耍
	Study,				// 学习
	Work				//	工作
};

// ����״̬����
class PetState : public QObject
{
	Q_OBJECT

public:
	// ���캯��������״̬�������ͳ�������
	PetState(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
		:	QObject(parent),
			m_fsm(fsm), m_attr(attr), 
			m_updateTimer(new QTimer(this)) { }

	// ������
	virtual ~PetState() = default;

	// ���麯����ÿ��״̬����ʵ�ֵĺ��ķ���
	virtual void enter() = 0;						// ����״̬
	virtual void update() = 0;					// ״̬����
	virtual void exit() = 0;							// �˳�״̬
	virtual PetStateType getType() = 0;	// ��ȡ״̬����

protected:
	PetFSM* m_fsm;					// ״̬������
	PetAttribute* m_attr;			//	��������
	QTimer* m_updateTimer;	//	״̬���¶�ʱ��
};

