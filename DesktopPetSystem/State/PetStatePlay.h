
#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "Config/PetConfig.h"

// 玩耍状态类
class PetStatePlay : public PetState
{
    Q_OBJECT

public:
    PetStatePlay(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStatePlay() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Play; }

private:
    PetConfig* m_config = PetConfig::getInstance();  // 配置实例
    int m_remPlayMood = 0;
    int m_remPlayEnergy = 0;
    int m_remPlayHunger = 0;
    int m_remPlayExp = 0;
};
