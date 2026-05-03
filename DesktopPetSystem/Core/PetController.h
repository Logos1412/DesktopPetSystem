#pragma once
#pragma execution_character_set("utf-8")

#include <QString>

class PetFSM;
class PetAttribute;
struct FoodData;

class PetController
{
public:
    PetController(PetFSM* fsm, PetAttribute* attr);

    bool handleMenuAction(const QString& action) const;
    bool handleFoodSelection(const QString& foodId) const;

private:
    bool applyFood(const FoodData& food) const;

private:
    PetFSM* m_petFsm = nullptr;
    PetAttribute* m_petAttr = nullptr;
};
