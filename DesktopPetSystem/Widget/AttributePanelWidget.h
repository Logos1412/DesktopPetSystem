#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>

class PetAttribute;

class AttributePanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AttributePanelWidget(PetAttribute* attr, QWidget* parent = nullptr);

    void showAtPos(const QPoint& pos);
    void refreshData();

signals:
    void mouseEntered();
    void mouseLeft();

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void initStyle();
    void initUI();

    PetAttribute* m_attr = nullptr;

    QLabel* m_levelLabel = nullptr;
    QLabel* m_expLabel = nullptr;
    QLabel* m_coinLabel = nullptr;

    QProgressBar* m_hungerBar = nullptr;
    QProgressBar* m_energyBar = nullptr;
    QProgressBar* m_moodBar = nullptr;

    QLabel* m_hungerValue = nullptr;
    QLabel* m_energyValue = nullptr;
    QLabel* m_moodValue = nullptr;
};
