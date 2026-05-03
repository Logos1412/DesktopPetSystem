#pragma once
#pragma execution_character_set("utf-8")

#include <QString>
#include <qtimer.h>
#include <qobject.h>
#include "PetAttribute.h"

class PetFSM;

// 宠物状态类型枚举
enum class PetStateType {
    Idle,           // 正常待机
    AbnormalIdle,   // 异常待机（属性过低）
    Eat,            // 进食
    Sleep,          // 睡眠
    Play,           // 玩耍
    Study,          // 学习
    Work,           // 工作
    Chat            // 聊天
};

// 异常原因枚举
enum class AbnormalReason {
    None,       // 无异常
    Hungry,     // 饥饿
    Sleepy,     // 困倦
    Sad         // 悲伤
};

// 宠物状态基类
class PetState : public QObject
{
    Q_OBJECT

public:
    PetState(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : QObject(parent),
          m_fsm(fsm), m_attr(attr),
          m_updateTimer(new QTimer(this)) { }

    virtual ~PetState() = default;

    // 进入状态
    virtual void enter() = 0;
    // 状态更新
    virtual void update() = 0;
    // 退出状态
    virtual void exit() = 0;
    // 获取状态类型
    virtual PetStateType getType() = 0;

    // 双击交互（默认空实现）
    virtual void onDoubleClick() {}

signals:
    // 请求播放动画信号
    void requestPlayAnimation(const QString& animationPath, bool isStateAnimation = false);

protected:
    /** 配置的 rat 为每分钟变化整数；每秒 tick 累加 remainder，整数部分应用到属性（进入状态时请将 remainder 置 0） */
    static int slicePerSecondFromRatePerMinute(int ratePerMinute, int& remainder);

    /** 进入状态时清零并记录「本窗口」起始三维，用于 60s 结算差值 */
    void resetPeriodicSettlementLog();
    /** 每秒 tick 调用一次：满 60 秒输出一条 [结算]；中段为预计变化（配置「每分钟」意图），结算前后为实际快照 */
    void maybeLogMinuteSettlement(const QString& stateLabel, int expectedDh, int expectedDe, int expectedDm);

    /** 「饱食+2」「精力-1」用于结算日志 */
    static QString formatSettlementAttrDelta(const QString& label, int delta);

    PetFSM* m_fsm;              // 状态机指针
    PetAttribute* m_attr;       // 属性指针
    QTimer* m_updateTimer;      // 状态更新定时器

private:
    int m_periodicSettlementTicks = 0;
    int m_settlementBaseHunger = 0;
    int m_settlementBaseEnergy = 0;
    int m_settlementBaseMood = 0;
};
