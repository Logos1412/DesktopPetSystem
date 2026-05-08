#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QTimer>
#include <QMap>
#include "PetState.h"

class PetAttribute;

// 宠物状态机类
class PetFSM : public QObject
{
    Q_OBJECT

public:
    PetFSM(PetAttribute* attr, QObject* parent = nullptr);
    ~PetFSM();

    // 切换状态
    void changeState(PetStateType stateType);
    // 获取当前状态
    PetStateType currentState() const { return m_currentStateType; }
    // 获取当前状态对象
    PetState* currentStateObject() const { return m_currentState; }
    // 按类型取状态实例（不改变当前状态；用于进餐前预设数据等）
    PetState* stateObject(PetStateType stateType) const { return m_states.value(stateType, nullptr); }
    // 状态更新
    void onStateUpdate();
    // 双击事件
    void onDoubleClick();

    /** 进食 GIF 播完一轮后由界面调用 */
    void notifyEatAnimationFinished();
    /** 入睡第一段 GIF 播完一轮后由界面调用 */
    void notifySleepFallAsleepFinished();

signals:
    // 状态改变信号
    void stateChanged(PetStateType state);
    // 状态切换完成信号（用于保存数据）
    void stateTransition();
    // 播放动画信号
    void playAnimation(const QString& animationPath, bool isStateAnimation = true, bool playOnce = false);

    // 聊天相关信号
    void showChatWidget();
    void showChatReply(const QString& reply);
    void chatOllamaPrefetchFailed(const QString& message);
    void chatBusyChanged(bool busy);
    void chatAssistantStarted();
    void chatAssistantDelta(const QString& text);
    void chatAssistantFinished(bool success, bool userCancelled, const QString& errorMessage);

private:
    // 初始化所有状态
    void initStates();
    // 检查是否需要切换到异常待机
    AbnormalReason checkAbnormal();

private:
    PetAttribute* m_attr = nullptr;                     // 属性指针
    QMap<PetStateType, PetState*> m_states;             // 状态映射
    PetState* m_currentState = nullptr;                 // 当前状态
    PetStateType m_currentStateType = PetStateType::Idle;   // 当前状态类型（初始值用于首次进入）
    bool m_isFirstEnter = true;                             // 是否首次进入状态
};
