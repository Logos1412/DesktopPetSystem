#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetFSM.h"

class PetStateStudy : public PetState
{
	Q_OBJECT

public:
	PetStateStudy(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
		: PetState(fsm, attr, parent) {
	}

	~PetStateStudy() override = default;

	void enter() override;
	void update() override;
	void exit() override;
	PetStateType getType() override { return PetStateType::Study; }

private:
	void checkStateSwitch();

	// 学习状态配置
	const int ENERGY_DECAY = 2;  // 精力-2/秒
	const int HUNGER_DECAY = 1;  // 饱食-1/秒
	const int MOOD_DECAY = 2;    // 心情-2/秒
	const int EXP_ADD = 5;       // 经验+5/秒
	const int MIN_ENERGY = 5;    // 精力<5退出
};
