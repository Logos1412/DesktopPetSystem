#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetFSM.h"

// 待机状态
class PetStateIdle : public PetState
{
	Q_OBJECT

public:
	PetStateIdle(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
		:	PetState(fsm, attr, parent) {}

	~PetStateIdle() override = default;

	// 实现基类纯虚函数
	void enter() override;			// 进入
	void update() override;		// 更新
	void exit() override;				// 退出
	PetStateType getType() override { return PetStateType::Idle; }

private:
	//检测是否需要切换到其他状态
	void checkStateSwitch();
};

