#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QPushButton>
#include "PetFSM.h"
#include "PetAttribute.h"

class PetMenuWidget	: public QWidget
{
	Q_OBJECT

public:
	// 构造函数 关联FSM和属性类
	explicit PetMenuWidget(PetFSM* fsm, PetAttribute* attr, QWidget* parent = nullptr);

	~PetMenuWidget() override = default;

	// 外部调用显示菜单
	void showAtPos(const QPoint& globalPos);

private:
    // 初始化菜单样式
    void initMenuStyle();

    // 初始化菜单按钮
    void initMenuButtons();

    // 初始化按钮布局
    void initMenuLayout();

    // 绑定按钮的点击逻辑
    void bindButtonClicked();

private:
    // 业务逻辑关联
    PetFSM* m_petFsm;
    PetAttribute* m_petAttr;
    // 互动下拉菜单
    QMenu* m_interactMenu;

    // 菜单按钮
    QPushButton* m_feedBtn;         // 喂食
    QPushButton* m_interactBtn;    // 互动
    QPushButton* m_wakeUpBtn;   // 唤醒
    QPushButton* m_exitBtn;          // 退出
};

