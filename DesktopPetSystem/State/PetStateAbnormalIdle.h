#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"

// 异常待机状态类
class PetStateAbnormalIdle : public PetState
{
    Q_OBJECT

public:
    PetStateAbnormalIdle(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStateAbnormalIdle() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::AbnormalIdle; }

    // 双击交互
    void onDoubleClick() override;

private:
    // 获取最低属性
    AbnormalReason getLowestAttribute();
    // 状态切换检查
    void checkStateSwitch();

    int m_remAbnormalHunger = 0;
    int m_remAbnormalEnergy = 0;
    int m_remAbnormalMood = 0;
};
