#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"

// 正常待机状态类
class PetStateIdle : public PetState
{
    Q_OBJECT

public:
    PetStateIdle(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStateIdle() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Idle; }

    // 双击交互
    void onDoubleClick() override;

private:
    // 状态切换检查
    void checkStateSwitch();

    /** 待机属性每分钟速率在每秒 tick 上的余数分摊 */
    int m_remIdleHunger = 0;
    int m_remIdleEnergy = 0;
    int m_remIdleMood = 0;
};
