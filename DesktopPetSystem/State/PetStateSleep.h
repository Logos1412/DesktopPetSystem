#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "Config/PetConfig.h"

// 睡眠状态类
class PetStateSleep : public PetState
{
    Q_OBJECT

public:
    PetStateSleep(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStateSleep() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Sleep; }

    // 双击交互
    void onDoubleClick() override;

    /** 入睡过渡 GIF 播完一轮后由 PetFSM / PetWidget 触发 */
    void onFallAsleepIntroFinished();

private:
    int m_sleepCount = 0;          // 睡眠计数
    bool m_isSleeping = false;     // 是否已切换到睡眠中循环动画（仅展示）
    PetConfig* m_config = PetConfig::getInstance();  // 配置实例
    int m_remSleepEnergy = 0;
    int m_remSleepHunger = 0;
};
