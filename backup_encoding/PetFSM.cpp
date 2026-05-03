#pragma execution_character_set("utf-8")

#include "PetFSM.h"
#include "PetStateIdle.h"
#include "PetStateAbnormalIdle.h"
#include "PetStateEat.h"
#include "PetStateSleep.h"
#include "PetStatePlay.h"
#include "PetStateStudy.h"
#include "PetStateWork.h"

#include <qdebug.h>

PetFSM::PetFSM(PetAttribute* attr, QObject* parent)
	:	QObject(parent), m_attr(attr)
{
	// ��ʼ��״̬
	initStates();

	// Ĭ�ϴ���
	switchState(PetStateType::Idle);
}

PetFSM::~PetFSM()
{
	// �ͷ�����״̬ʵ��
	qDeleteAll(m_stateMap);
	m_stateMap.clear();
}

// ��ʼ��״̬
void PetFSM::initStates()
{
	m_stateMap[PetStateType::Idle] = new PetStateIdle(this, m_attr, this);
	m_stateMap[PetStateType::AbnormalIdle] = new PetStateAbnormalIdle(this, m_attr, this);
	m_stateMap[PetStateType::Eat] = new PetStateEat(this, m_attr, this);
	m_stateMap[PetStateType::Sleep] = new PetStateSleep(this, m_attr, this);
	m_stateMap[PetStateType::Play] = new PetStatePlay(this, m_attr, this);
	m_stateMap[PetStateType::Study] = new PetStateStudy(this, m_attr, this);
	m_stateMap[PetStateType::Work] = new PetStateWork(this, m_attr, this);
}

// �л�״̬
void PetFSM::switchState(PetStateType type)
{
	// ��ǰ״ֱ̬�ӷ���
	if (m_currentStateType == type && m_currentState != nullptr) {
		return;
	}

	// ��״̬�˳�
	if (m_currentState != nullptr) {
		m_currentState->exit();
	}

	// ��״̬����
	PetState* newState = m_stateMap.value(type, nullptr);
	if (newState == nullptr) {
		qWarning() << "״̬�����ڣ�" << static_cast<int>(type);
		return;
	}
	m_currentState = newState;
	m_currentStateType = type;
	m_currentState->enter();

	qDebug() << "�л�״̬��" << static_cast<int>(m_currentStateType);

	emit stateChanged(m_currentStateType);
}

// ��ʱ����
void PetFSM::onStateUpdate()
{
	if (m_currentState != nullptr) {
		m_currentState->update();
	}
}
