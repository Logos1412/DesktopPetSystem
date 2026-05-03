#pragma once
#pragma execution_character_set("utf-8")

#include "PetState.h"
#include "PetFSM.h"

class PetStateEat : public PetState
{
    Q_OBJECT

public:
    PetStateEat(PetFSM* fsm, PetAttribute* attr, QObject* parent = nullptr)
        : PetState(fsm, attr, parent) {
    }

    ~PetStateEat() override = default;

    void enter() override;
    void update() override;
    void exit() override;
    PetStateType getType() override { return PetStateType::Eat; }

    void setFoodBonus(int hungerBonus, int moodBonus, int energyBonus);

private:
    void checkStateSwitch();

private:
    int m_eatDuration = 0;
    const int EAT_DURATION = 3;

    int m_hungerBonus = 30;
    int m_moodBonus = 5;
    int m_energyBonus = 5;
};
