#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetFSM.h"

class PetStatePlay : public PetState
{
	Q_OBJECT

public:
	
	PetStatePlay(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
		:	 PetState(fsm, attr, parent) { }

	~PetStatePlay() override = default;

	void enter() override;
	void update() override;
	void exit() override;
	// 返回对应状态枚举
	PetStateType getType() override { return PetStateType::Play; }

private:
	// 状态切换检查（和你的sleep类一致）
	void checkStateSwitch();

	// 玩耍状态配置（硬编码，后续可移到配置文件）
	const int ENERGY_DECAY = 2;  // 精力-3/秒
	const int HUNGER_DECAY = 3;  // 饱食-2/秒
	const int MOOD_ADD = 5;      // 心情+5/秒
	const int MIN_ENERGY = 10;   // 精力<10退出
	const int MAX_MOOD = 100;    // 心情≥100退出
};
