#pragma execution_character_set("utf-8")

#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetStateIdle.h"
#include "PetStateAbnormalIdle.h"
#include "PetStateEat.h"
#include "PetStateSleep.h"
#include "PetStatePlay.h"
#include "PetStateStudy.h"
#include "PetStateWork.h"
#include "../State/PetStateChat.h"

#include <QDebug>

// 构造函数
PetFSM::PetFSM(PetAttribute* attr, QObject* parent)
    : QObject(parent), m_attr(attr)
{
    // 注册元类型
    qRegisterMetaType<PetStateType>("PetStateType");

    initStates();
    changeState(PetStateType::Idle);
}

// 析构函数
PetFSM::~PetFSM()
{
    for (auto state : m_states) {
        if (state) {
            delete state;
        }
    }
    m_states.clear();
}

// 初始化所有状态
void PetFSM::initStates()
{
    m_states[PetStateType::Idle] = new PetStateIdle(this, m_attr);
    m_states[PetStateType::AbnormalIdle] = new PetStateAbnormalIdle(this, m_attr);
    m_states[PetStateType::Eat] = new PetStateEat(this, m_attr);
    m_states[PetStateType::Sleep] = new PetStateSleep(this, m_attr);
    m_states[PetStateType::Play] = new PetStatePlay(this, m_attr);
    m_states[PetStateType::Study] = new PetStateStudy(this, m_attr);
    m_states[PetStateType::Work] = new PetStateWork(this, m_attr);
    m_states[PetStateType::Chat] = new PetStateChat(this, m_attr);

    // 连接所有状态的动画请求信号
    for (auto state : m_states) {
        connect(state, &PetState::requestPlayAnimation, this, &PetFSM::playAnimation);
    }

    // 连接聊天状态的信号
    PetStateChat* chatState = dynamic_cast<PetStateChat*>(m_states[PetStateType::Chat]);
    if (chatState) {
        connect(chatState, &PetStateChat::showChatWidget, this, &PetFSM::showChatWidget);
        connect(chatState, &PetStateChat::showChatReply, this, &PetFSM::showChatReply);
        connect(chatState, &PetStateChat::chatOllamaPrefetchFailed, this, &PetFSM::chatOllamaPrefetchFailed);
        connect(chatState, &PetStateChat::chatBusyChanged, this, &PetFSM::chatBusyChanged);
        connect(chatState, &PetStateChat::chatAssistantStarted, this, &PetFSM::chatAssistantStarted);
        connect(chatState, &PetStateChat::chatAssistantDelta, this, &PetFSM::chatAssistantDelta);
        connect(chatState, &PetStateChat::chatAssistantFinished, this, &PetFSM::chatAssistantFinished);
    }

    qDebug() << "[状态机] 所有状态初始化完成";
}

// 切换状态
void PetFSM::changeState(PetStateType stateType)
{
    // 如果目标状态不存在
    if (!m_states.contains(stateType)) {
        qWarning() << "[状态机错误] 目标状态不存在:" << static_cast<int>(stateType);
        return;
    }

    // 如果是相同状态且不是首次进入，不切换
    if (!m_isFirstEnter && m_currentStateType == stateType) {
        qDebug() << "[状态机] 已处于目标状态，跳过切换";
        return;
    }

    // 退出当前状态
    if (m_currentState) {
        qDebug() << "[状态机] 退出状态:" << static_cast<int>(m_currentStateType);
        m_currentState->exit();
        emit stateTransition();
    }

    // 切换到新状态
    m_currentState = m_states[stateType];
    m_currentStateType = stateType;
    m_isFirstEnter = false;

    // 进入新状态
    qDebug() << "[状态机] 进入状态:" << static_cast<int>(stateType);
    m_currentState->enter();

    // 发射状态改变信号
    emit stateChanged(stateType);
}

// 状态更新
void PetFSM::onStateUpdate()
{
    if (m_currentState) {
        m_currentState->update();
    }
}

// 双击事件
void PetFSM::onDoubleClick()
{
    if (m_currentState) {
        m_currentState->onDoubleClick();
    }
}

void PetFSM::notifyEatAnimationFinished()
{
    if (m_currentStateType != PetStateType::Eat)
        return;
    changeState(PetStateType::Idle);
}

void PetFSM::notifySleepFallAsleepFinished()
{
    if (m_currentStateType != PetStateType::Sleep)
        return;
    PetStateSleep* sleepState = qobject_cast<PetStateSleep*>(m_currentState);
    if (sleepState)
        sleepState->onFallAsleepIntroFinished();
}

// 检查是否需要切换到异常待机
AbnormalReason PetFSM::checkAbnormal()
{
    int hunger = m_attr->getHunger();
    int energy = m_attr->getEnergy();
    int mood = m_attr->getMood();

    // 检查各项属性是否过低
    if (hunger <= 20) return AbnormalReason::Hungry;
    if (energy <= 20) return AbnormalReason::Sleepy;
    if (mood <= 20) return AbnormalReason::Sad;

    return AbnormalReason::None;
}
