#pragma once
#pragma execution_character_set("utf-8")

#include <QDialog>

class QListWidget;
class QStackedWidget;

/** 可视化编辑 pet_config.json，保存后触发热更新（PetConfig::loadConfig） */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override;

signals:
    /** 在设置窗口内保存了聊天记忆文件后发出，供正在对话的状态重载记忆 */
    void chatMemoryEdited();

private slots:
    void onSave();
    void onReload();
    void onRestoreDefaults();
    void onApplyChatRuntimePreset();

private:
    QString resolveDefaultsTemplatePath() const;
    void reloadFromDisk();
    void buildTabs();
    QJsonObject collectRootJson() const;
    void applyRootJson(const QJsonObject& root);

    QListWidget* m_categoryList = nullptr;
    QStackedWidget* m_pageStack = nullptr;
    QString m_configPath;
    /** FormWidgets，定义见 SettingsDialog.cpp */
    void* m_formWidgets = nullptr;
};
