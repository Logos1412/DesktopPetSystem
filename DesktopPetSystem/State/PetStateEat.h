#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetConfig.h"

class PetStateEat : public PetState
{
    Q_OBJECT

public:
    PetStateEat(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {}

    ~PetStateEat() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Eat; }

    void setFood(const QString& foodId);
    void setFood(const FoodData& food);

private:
    FoodData m_currentFood;
    bool m_hasFood = false;
};
