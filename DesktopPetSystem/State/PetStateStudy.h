#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "Config/PetConfig.h"

// 学习状态类
class PetStateStudy : public PetState
{
    Q_OBJECT

public:
    PetStateStudy(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStateStudy() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Study; }

private:
    PetConfig* m_config = PetConfig::getInstance();  // 配置实例
    int m_remStudyEnergy = 0;
    int m_remStudyHunger = 0;
    int m_remStudyMood = 0;
    int m_remStudyExp = 0;
};
