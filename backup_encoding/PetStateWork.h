#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetFSM.h"

class PetStateWork : public PetState
{
	Q_OBJECT

public:
	PetStateWork(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
		: PetState(fsm, attr, parent) {
	}

	~PetStateWork() override = default;

	void enter() override;
	void update() override;
	void exit() override;
	PetStateType getType() override { return PetStateType::Work; }

private:
	void checkStateSwitch();

	// 工作状态配置
	const int ENERGY_DECAY = 3;  // 精力-3/秒
	const int HUNGER_DECAY = 3;  // 饱食-3/秒
	const int MOOD_DECAY = 4;    // 心情-4/秒
	const int EXP_ADD = 4;       // 经验+4/秒
	const int COIN_ADD = 6;      // 金币+6/秒
	const int MIN_ENERGY = 5;    // 精力<5退出
	const int MIN_HUNGER = 10;   // 饱食<10退出
};