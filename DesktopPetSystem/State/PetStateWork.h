#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "Config/PetConfig.h"

// 工作状态类
class PetStateWork : public PetState
{
    Q_OBJECT

public:
    PetStateWork(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStateWork() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Work; }

private:
    PetConfig* m_config = PetConfig::getInstance();  // 配置实例
    int m_remWorkEnergy = 0;
    int m_remWorkHunger = 0;
    int m_remWorkMood = 0;
    int m_remWorkExp = 0;
    int m_remWorkCoin = 0;
};
