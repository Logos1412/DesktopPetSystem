#pragma once
#pragma execution_character_set("utf-8")

#include <qobject.h>
#include <qmap.h>
#include "PetState.h"
#include "PetAttribute.h"

//前置说明
class PetState;

class PetFSM : public QObject
{
	Q_OBJECT

public:
	// 构造函数：关联宠物属性，初始化所有状态
	explicit PetFSM(PetAttribute* attr, QObject* parent = nullptr);
	~PetFSM();

	// 切换状态
	void switchState(PetStateType type);

	// 获取当前状态
	PetStateType currentState() const { return m_currentStateType; }

	// 获取状态实例
	PetState* getState(PetStateType type) const { return m_stateMap.value(type, nullptr); }

	/*
	信号
	*/
signals:
	void stateChanged(PetStateType newState);		// 状态改变

	/*
	槽函数
	*/
public slots:
		void onStateUpdate();		// 状态更新

private:          
	//初始化状态
	void initStates();

private:
	PetAttribute* m_attr;												//宠物属性
	PetState* m_currentState = nullptr;						//状态实例
	PetStateType m_currentStateType;							//状态类型
	QMap<PetStateType, PetState*> m_stateMap;		//状态映射表
};

