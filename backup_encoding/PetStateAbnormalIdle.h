#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetFSM.h"

class PetStateAbnormalIdle : public PetState
{
	Q_OBJECT

public:
	PetStateAbnormalIdle(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
		:	PetState(fsm, attr, parent){ }
	~PetStateAbnormalIdle() override = default;

	// 实现状态接口
	void enter() override;
	void update() override;
	void exit() override;
	PetStateType getType() override { return PetStateType::AbnormalIdle; }

private:
	// 检查是否需要切换状态
	void checkStateSwitch();
};

