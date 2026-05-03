#pragma execution_character_set("utf-8")

#include "PetController.h"

#include "PetFSM.h"
#include "PetAttribute.h"
#include "PetConfig.h"
#include "PetStateEat.h"

#include <QApplication>
#include <QDebug>

PetController::PetController(PetFSM* fsm, PetAttribute* attr)
    : m_petFsm(fsm), m_petAttr(attr)
{
}

bool PetController::handleMenuAction(const QString& action) const
{
    if (!m_petFsm || !m_petAttr) {
        return false;
    }

    if (action == u8"学习") {
        m_petFsm->changeState(PetStateType::Study);
        return true;
    }
    if (action == u8"玩耍") {
        m_petFsm->changeState(PetStateType::Play);
        return true;
    }
    if (action == u8"工作") {
        m_petFsm->changeState(PetStateType::Work);
        return true;
    }
    if (action == u8"睡觉") {
        m_petFsm->changeState(PetStateType::Sleep);
        return true;
    }
    if (action == u8"对话") {
        m_petFsm->changeState(PetStateType::Chat);
        return true;
    }
    if (action == u8"退出") {
        qApp->quit();
        return true;
    }

    return false;
}

bool PetController::handleFoodSelection(const QString& foodId) const
{
    if (m_petFsm && m_petFsm->currentState() == PetStateType::Eat) {
        qWarning() << "[控制器] 进食中，请勿重复购买";
        return false;
    }
    PetConfig* config = PetConfig::getInstance();
    if (!config->hasFood(foodId)) {
        qWarning() << "[控制器] 食物不存在:" << foodId;
        return false;
    }
    return applyFood(config->getFood(foodId));
}

bool PetController::applyFood(const FoodData& food) const
{
    if (!m_petFsm || !m_petAttr) {
        return false;
    }

    if (m_petFsm->currentState() == PetStateType::Eat) {
        qWarning() << "[控制器] 进食中，无法购买";
        return false;
    }

    /* 必须先取到 Eat 再给食物并 changeState：enter() 若 m_hasFood 为假会立即退回待机 */
    PetStateEat* eatState = dynamic_cast<PetStateEat*>(m_petFsm->stateObject(PetStateType::Eat));
    if (!eatState) {
        qWarning() << "[控制器] 进食状态未初始化";
        return false;
    }

    if (m_petAttr->getCoin() < food.price) {
        qWarning() << "[控制器] 金币不足，无法购买:" << food.name;
        return false;
    }

    m_petAttr->changeCoin(-food.price);
    eatState->setFood(food);

    if (m_petFsm->currentState() != PetStateType::Eat) {
        m_petFsm->changeState(PetStateType::Eat);
    }
    return true;
}
