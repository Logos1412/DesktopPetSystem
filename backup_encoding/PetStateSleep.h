#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetFSM.h"

class PetStateSleep : public PetState
{
	Q_OBJECT

public:
	// 对齐你的构造函数参数顺序：fsm → attr → parent
	PetStateSleep(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
		: PetState(fsm, attr, parent) {
	}

	~PetStateSleep() override = default;

	void enter() override;
	void update() override;
	void exit() override;
	// 返回睡眠状态枚举（和其他状态统一）
	PetStateType getType() override { return PetStateType::Sleep; }

	// 保留你原有的主动唤醒方法
	void wakeUp();

private:
	// 状态切换检查（和其他状态统一命名）
	void checkStateSwitch();

	// 睡眠状态配置（硬编码，后续可移到配置文件）
	const int ENERGY_ADD = 3;      // 精力+3/秒（和你原有逻辑一致）
	const int HUNGER_DECAY = 1;    // 饱食-1/秒（补充，符合宠物睡眠逻辑）
	const int MOOD_ADD = 2;      // 心情+2/秒（补充，符合宠物睡眠逻辑）
	const int MAX_ENERGY = 100;    // 精力≥100退出（和你原有逻辑一致）
};

