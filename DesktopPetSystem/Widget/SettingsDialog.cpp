#pragma execution_character_set("utf-8")

#include "SettingsDialog.h"
#include "PetConfig.h"

#include <QSplitter>
#include <QListWidget>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QSize>
#include <QAbstractItemView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QVariantMap>
#include <QDir>
#include <QDialog>
#include <QHash>

namespace {

static QString projectRootFromPetConfigPath(const QString& petConfigPath)
{
    if (petConfigPath.isEmpty()) {
        return {};
    }
    QDir d(QFileInfo(petConfigPath).absolutePath());
    if (!d.cdUp()) {
        return {};
    }
    if (!d.cdUp()) {
        return {};
    }
    return d.absolutePath();
}

static QString chatMemoryAbsolutePathForForm()
{
    PetConfig* cfg = PetConfig::getInstance();
    const QString petCfg = cfg->getConfigFilePath();
    const QString root = projectRootFromPetConfigPath(petCfg);
    if (root.isEmpty()) {
        return {};
    }
    return QDir(root).absoluteFilePath(cfg->getChatMemoryPath());
}

struct AnimScaleUiRow {
    const char* relPath;
    const char* labelZh;
};

static const AnimScaleUiRow kAnimScaleRows[] = {
    {"Idle/Idle.gif", "待机·站立"},
    {"Idle/double_click/Angry.gif", "待机·双击随机·生气"},
    {"Idle/double_click/Laugh.gif", "待机·双击随机·笑"},
    {"Idle/double_click/Shy.gif", "待机·双击随机·害羞"},
    {"Move/Move_L.gif", "随机移动·向左"},
    {"Move/Move_R.gif", "随机移动·向右"},
    {"AbnormalIdle/AbnormalIdle.gif", "异常待机"},
    {"AbnormalIdle/double_click/Hungry.gif", "异常待机·双击·饥饿"},
    {"AbnormalIdle/double_click/Sleepy.gif", "异常待机·双击·困倦"},
    {"AbnormalIdle/double_click/Sad.gif", "异常待机·双击·低落"},
    {"Eat/Eat.gif", "进食"},
    {"Sleep/Sleep.gif", "睡眠·入睡过渡"},
    {"Sleep/Sleeping.gif", "睡眠·熟睡循环"},
    {"Play/Play.gif", "玩耍"},
    {"Study/Study.gif", "学习"},
    {"Work/Work.gif", "工作"},
    {"Chat/Chat.gif", "聊天"},
};

struct FoodRowWidgets {
    QString id;
    QGroupBox* box = nullptr;
    QLineEdit* name = nullptr;
    QSpinBox* price = nullptr;
    QSpinBox* hunger = nullptr;
    QSpinBox* energy = nullptr;
    QSpinBox* mood = nullptr;
    QSpinBox* exp = nullptr;
};

struct FormWidgets {
    /* attributes */
    QSpinBox* a_default_level = nullptr;
    QSpinBox* a_default_exp = nullptr;
    QSpinBox* a_default_hunger = nullptr;
    QSpinBox* a_default_energy = nullptr;
    QSpinBox* a_default_mood = nullptr;
    QSpinBox* a_hunger_decay_idle = nullptr;
    QSpinBox* a_energy_decay_idle = nullptr;
    QSpinBox* a_mood_decay_idle = nullptr;
    QSpinBox* a_hunger_decay_abnormal = nullptr;
    QSpinBox* a_energy_decay_abnormal = nullptr;
    QSpinBox* a_mood_decay_abnormal = nullptr;

    QSpinBox* t_abnormal = nullptr;
    QSpinBox* t_auto_sleep_energy = nullptr;
    QSpinBox* t_recovery = nullptr;

    QSpinBox* s_energy_recovery_pm = nullptr;
    QSpinBox* s_hunger_decay_pm = nullptr;

    QSpinBox* p_mood_inc = nullptr;
    QSpinBox* p_energy_dec = nullptr;
    QSpinBox* p_hunger_dec = nullptr;
    QSpinBox* p_exp_gain = nullptr;

    QSpinBox* w_energy_dec = nullptr;
    QSpinBox* w_hunger_dec = nullptr;
    QSpinBox* w_mood_dec = nullptr;
    QSpinBox* w_exp_gain = nullptr;
    QSpinBox* w_coin_gain = nullptr;

    QSpinBox* st_energy_dec = nullptr;
    QSpinBox* st_hunger_dec = nullptr;
    QSpinBox* st_mood_dec = nullptr;
    QSpinBox* st_exp_gain = nullptr;

    QSpinBox* c_energy_cost = nullptr;
    QSpinBox* c_hunger_cost = nullptr;
    QSpinBox* c_mood_gain = nullptr;

    QSpinBox* g_exp_base = nullptr;
    QSpinBox* g_exp_growth = nullptr;

    QComboBox* cr_provider = nullptr;
    QLineEdit* cr_api_base = nullptr;
    QLineEdit* cr_api_key = nullptr;
    QLineEdit* cr_api_key_env = nullptr;
    QLineEdit* cr_model = nullptr;
    QLineEdit* cr_script_path = nullptr;
    QLineEdit* cr_python_path = nullptr;
    QLineEdit* cr_ollama_host = nullptr;
    QSpinBox* cr_context_turns = nullptr;
    QSpinBox* cr_max_context_chars = nullptr;
    QSpinBox* cr_max_reply_tokens = nullptr;
    QSpinBox* cr_max_input_chars = nullptr;
    QSpinBox* cr_context_reserve = nullptr;
    QSpinBox* cr_max_hist_est_tokens = nullptr;
    QSpinBox* cr_chars_per_est_token = nullptr;
    QSpinBox* cr_summary_memory_chars = nullptr;
    QCheckBox* cr_no_stage_directions = nullptr;
    QPlainTextEdit* cr_pet_persona = nullptr;
    QLineEdit* cr_memory_path = nullptr;
    QCheckBox* cr_summary_enabled = nullptr;
    QSpinBox* cr_summary_compress_after = nullptr;
    QSpinBox* cr_summary_keep_recent = nullptr;
    QSpinBox* cr_summary_max_chars = nullptr;
    QSpinBox* cr_mem_pref = nullptr;
    QSpinBox* cr_mem_tasks = nullptr;
    QSpinBox* cr_mem_avoid = nullptr;
    QSpinBox* cr_mem_facts = nullptr;

    QComboBox* cr_chat_preset = nullptr;
    QPushButton* cr_chat_preset_apply = nullptr;

    QCheckBox* log_verbose = nullptr;

    QSpinBox* m_idle_timeout = nullptr;
    QDoubleSpinBox* m_move_speed = nullptr;
    QSpinBox* m_update_interval = nullptr;
    QSpinBox* m_direction_keep = nullptr;

    QSpinBox* i_dc_cooldown_ms = nullptr;
    QSpinBox* i_dc_mood_idle = nullptr;
    QSpinBox* i_dc_mood_abnormal = nullptr;

    QSpinBox* win_width = nullptr;
    QSpinBox* win_height = nullptr;

    QSpinBox* l_max = nullptr;
    QSpinBox* l_min = nullptr;

    QWidget* foodPage = nullptr;
    QVBoxLayout* foodOuterLayout = nullptr;
    QVBoxLayout* foodListLayout = nullptr;
    QVector<FoodRowWidgets> foodRows;

    QPlainTextEdit* cm_pref = nullptr;
    QPlainTextEdit* cm_tasks = nullptr;
    QPlainTextEdit* cm_avoid = nullptr;
    QPlainTextEdit* cm_facts = nullptr;
    QLabel* cm_path = nullptr;
    QLabel* cm_msgs = nullptr;

    QHash<QString, QDoubleSpinBox*> anim_scale_spin;
};

static void loadChatMemoryIntoForm(FormWidgets* F)
{
    if (!F || !F->cm_pref) {
        return;
    }
    const QString path = chatMemoryAbsolutePathForForm();
    F->cm_path->setText(path.isEmpty() ? QStringLiteral("（未知路径）") : path);
    if (path.isEmpty() || !QFile::exists(path)) {
        F->cm_pref->clear();
        F->cm_tasks->clear();
        F->cm_avoid->clear();
        F->cm_facts->clear();
        F->cm_msgs->setText(QStringLiteral("尚无记忆文件；保存设置时将按本页内容创建。"));
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        F->cm_msgs->setText(QStringLiteral("读取失败：") + f.errorString());
        return;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        F->cm_msgs->setText(QStringLiteral("JSON 无效：") + err.errorString());
        return;
    }
    const QJsonObject root = doc.object();
    const QJsonObject mem = root.value(QStringLiteral("memory")).toObject();
    F->cm_pref->setPlainText(mem.value(QStringLiteral("preferences")).toString());
    F->cm_tasks->setPlainText(mem.value(QStringLiteral("tasks")).toString());
    F->cm_avoid->setPlainText(mem.value(QStringLiteral("avoid")).toString());
    F->cm_facts->setPlainText(mem.value(QStringLiteral("facts")).toString());
    const int n = root.value(QStringLiteral("messages")).toArray().size();
    F->cm_msgs->setText(QStringLiteral("对话消息条数：%1（本页不编辑消息列表）").arg(n));
}

static bool saveChatMemoryFromForm(FormWidgets* F)
{
    if (!F || !F->cm_pref) {
        return false;
    }
    PetConfig* cfg = PetConfig::getInstance();
    const QString path = chatMemoryAbsolutePathForForm();
    if (path.isEmpty()) {
        return false;
    }
    const QJsonObject lim = cfg->getStructuredMemoryLimitsJson();
    auto clip = [&lim](const QString& s, const QString& key) -> QString {
        const int mx = lim.value(key).toInt(120);
        if (s.size() <= mx) {
            return s;
        }
        return s.left(mx);
    };

    QJsonObject root;
    if (QFile::exists(path)) {
        QFile inf(path);
        if (!inf.open(QIODevice::ReadOnly)) {
            return false;
        }
        QJsonParseError e;
        const QJsonDocument d = QJsonDocument::fromJson(inf.readAll(), &e);
        inf.close();
        if (e.error != QJsonParseError::NoError || !d.isObject()) {
            return false;
        }
        root = d.object();
    } else {
        root.insert(QStringLiteral("version"), 2);
        root.insert(QStringLiteral("messages"), QJsonArray());
        root.insert(QStringLiteral("summary"), QString());
    }

    QJsonObject mem;
    mem.insert(QStringLiteral("preferences"), clip(F->cm_pref->toPlainText(), QStringLiteral("preferences")));
    mem.insert(QStringLiteral("tasks"), clip(F->cm_tasks->toPlainText(), QStringLiteral("tasks")));
    mem.insert(QStringLiteral("avoid"), clip(F->cm_avoid->toPlainText(), QStringLiteral("avoid")));
    mem.insert(QStringLiteral("facts"), clip(F->cm_facts->toPlainText(), QStringLiteral("facts")));
    root.insert(QStringLiteral("memory"), mem);
    root.insert(QStringLiteral("version"), 2);
    root.insert(QStringLiteral("summary"), QString());

    const QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    out.close();
    return true;
}

static QSpinBox* spinInt(QWidget* parent, int lo, int hi)
{
    auto* s = new QSpinBox(parent);
    s->setRange(lo, hi);
    return s;
}

static QWidget* makeSectionHint(QWidget* parent, const QString& text)
{
    auto* lab = new QLabel(text, parent);
    lab->setWordWrap(true);
    lab->setStyleSheet(QStringLiteral("color: #666; font-size: 11px;"));
    return lab;
}

static void clearFoodRows(FormWidgets* F)
{
    for (FoodRowWidgets& r : F->foodRows) {
        if (r.box) {
            delete r.box;
            r.box = nullptr;
        }
    }
    F->foodRows.clear();
}

static void appendFoodRow(FormWidgets* F, const QString& foodId, const QJsonObject& foodObj)
{
    FoodRowWidgets row;
    row.id = foodId;

    row.box = new QGroupBox(QStringLiteral("%1（%2）").arg(foodObj.value(QStringLiteral("name")).toString(), foodId), F->foodPage);
    auto* form = new QFormLayout(row.box);

    row.name = new QLineEdit(row.box);
    row.name->setText(foodObj.value(QStringLiteral("name")).toString());
    form->addRow(QStringLiteral("显示名称"), row.name);

    row.price = spinInt(row.box, 0, 999999);
    row.price->setValue(foodObj.value(QStringLiteral("price")).toInt(0));
    form->addRow(QStringLiteral("价格（金币）"), row.price);

    row.hunger = spinInt(row.box, 0, 999);
    row.hunger->setValue(foodObj.value(QStringLiteral("hunger")).toInt(0));
    form->addRow(QStringLiteral("饱食回复"), row.hunger);

    row.energy = spinInt(row.box, 0, 999);
    row.energy->setValue(foodObj.value(QStringLiteral("energy")).toInt(0));
    form->addRow(QStringLiteral("精力回复"), row.energy);

    row.mood = spinInt(row.box, 0, 999);
    row.mood->setValue(foodObj.value(QStringLiteral("mood")).toInt(0));
    form->addRow(QStringLiteral("心情回复"), row.mood);

    row.exp = spinInt(row.box, 0, 999);
    row.exp->setValue(foodObj.value(QStringLiteral("exp")).toInt(0));
    form->addRow(QStringLiteral("经验奖励"), row.exp);

    if (F->foodListLayout) {
        F->foodListLayout->addWidget(row.box);
    }
    F->foodRows.append(row);
}

static QJsonObject nested(const QJsonObject& root, const QString& key)
{
    QJsonValue v = root.value(key);
    return v.isObject() ? v.toObject() : QJsonObject();
}

static QString chatRuntimePresetsPath(const QString& configPath)
{
    if (configPath.isEmpty()) {
        return {};
    }
    return QFileInfo(configPath).absolutePath() + QStringLiteral("/chat_runtime_presets.json");
}

static QVariantMap chatRuntimePresetTierBeginner()
{
    QVariantMap m;
    m.insert(QStringLiteral("context_turns"), 6);
    m.insert(QStringLiteral("max_context_chars"), 4800);
    m.insert(QStringLiteral("max_reply_tokens"), 768);
    m.insert(QStringLiteral("max_input_chars"), 600);
    m.insert(QStringLiteral("summary_compress_after_turns"), 10);
    m.insert(QStringLiteral("summary_keep_recent_turns"), 6);
    m.insert(QStringLiteral("summary_max_chars"), 280);
    m.insert(QStringLiteral("memory_preferences_max_chars"), 72);
    m.insert(QStringLiteral("memory_tasks_max_chars"), 72);
    m.insert(QStringLiteral("memory_avoid_max_chars"), 56);
    m.insert(QStringLiteral("memory_facts_max_chars"), 72);
    return m;
}

static QVariantMap chatRuntimePresetTierMid()
{
    QVariantMap m;
    m.insert(QStringLiteral("context_turns"), 10);
    m.insert(QStringLiteral("max_context_chars"), 11000);
    m.insert(QStringLiteral("max_reply_tokens"), 1536);
    m.insert(QStringLiteral("max_input_chars"), 900);
    m.insert(QStringLiteral("summary_compress_after_turns"), 15);
    m.insert(QStringLiteral("summary_keep_recent_turns"), 8);
    m.insert(QStringLiteral("summary_max_chars"), 400);
    m.insert(QStringLiteral("memory_preferences_max_chars"), 96);
    m.insert(QStringLiteral("memory_tasks_max_chars"), 96);
    m.insert(QStringLiteral("memory_avoid_max_chars"), 72);
    m.insert(QStringLiteral("memory_facts_max_chars"), 96);
    return m;
}

static QVariantMap chatRuntimePresetTierAdvanced()
{
    QVariantMap m;
    m.insert(QStringLiteral("context_turns"), 14);
    m.insert(QStringLiteral("max_context_chars"), 28000);
    m.insert(QStringLiteral("max_reply_tokens"), 2048);
    m.insert(QStringLiteral("max_input_chars"), 1200);
    m.insert(QStringLiteral("summary_compress_after_turns"), 22);
    m.insert(QStringLiteral("summary_keep_recent_turns"), 10);
    m.insert(QStringLiteral("summary_max_chars"), 560);
    m.insert(QStringLiteral("memory_preferences_max_chars"), 120);
    m.insert(QStringLiteral("memory_tasks_max_chars"), 120);
    m.insert(QStringLiteral("memory_avoid_max_chars"), 96);
    m.insert(QStringLiteral("memory_facts_max_chars"), 120);
    return m;
}

static void populateChatRuntimePresetCombo(QComboBox* combo, const QString& configPath)
{
    combo->clear();
    combo->addItem(QStringLiteral("— 选择档位 —"), QVariant());

    QJsonArray presets;
    const QString path = chatRuntimePresetsPath(configPath);
    if (!path.isEmpty() && QFile::exists(path)) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray raw = file.readAll();
            file.close();
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
            if (!doc.isNull() && doc.isObject()) {
                presets = doc.object().value(QStringLiteral("presets")).toArray();
            }
        }
    }

    if (!presets.isEmpty()) {
        for (const QJsonValue& v : presets) {
            const QJsonObject o = v.toObject();
            const QString title = o.value(QStringLiteral("title")).toString().trimmed();
            const QJsonObject values = o.value(QStringLiteral("values")).toObject();
            if (title.isEmpty() || values.isEmpty()) {
                continue;
            }
            QVariantMap vm;
            for (auto it = values.begin(); it != values.end(); ++it) {
                vm.insert(it.key(), it.value().toVariant());
            }
            combo->addItem(title, QVariant(vm));
            const QString hint = o.value(QStringLiteral("hint")).toString().trimmed();
            if (!hint.isEmpty()) {
                combo->setItemData(combo->count() - 1, hint, Qt::ToolTipRole);
            }
        }
        return;
    }

    combo->addItem(QStringLiteral("初级 · 本地小模型（参考 Llama 3.1 8B）"),
                   QVariant(chatRuntimePresetTierBeginner()));
    combo->setItemData(
        combo->count() - 1,
        QStringLiteral("适合本机 Ollama 小显存；上下文与摘要偏保守，更早压缩历史。"),
        Qt::ToolTipRole);
    combo->addItem(QStringLiteral("中级 · 均衡（参考 Qwen2.5-32B-Instruct）"),
                   QVariant(chatRuntimePresetTierMid()));
    combo->setItemData(combo->count() - 1,
                       QStringLiteral("介于本地 8B 与云端大上下文之间；适合中端显卡或中等预算 API。"),
                       Qt::ToolTipRole);
    combo->addItem(QStringLiteral("高级 · 大上下文 API（参考 DeepSeek V4-Flash）"),
                   QVariant(chatRuntimePresetTierAdvanced()));
    combo->setItemData(combo->count() - 1,
                       QStringLiteral("适合长上下文云端模型；历史更长、较晚触发摘要。"),
                       Qt::ToolTipRole);
}

static void applyChatRuntimePresetMap(FormWidgets* F, const QVariantMap& m)
{
    if (!F || m.isEmpty()) {
        return;
    }
    auto setSpin = [&m](QSpinBox* box, const QString& key) {
        if (!box || !m.contains(key)) {
            return;
        }
        box->setValue(m.value(key).toInt());
    };
    setSpin(F->cr_context_turns, QStringLiteral("context_turns"));
    setSpin(F->cr_max_context_chars, QStringLiteral("max_context_chars"));
    setSpin(F->cr_max_reply_tokens, QStringLiteral("max_reply_tokens"));
    setSpin(F->cr_max_input_chars, QStringLiteral("max_input_chars"));
    setSpin(F->cr_summary_compress_after, QStringLiteral("summary_compress_after_turns"));
    setSpin(F->cr_summary_keep_recent, QStringLiteral("summary_keep_recent_turns"));
    setSpin(F->cr_summary_max_chars, QStringLiteral("summary_max_chars"));
    setSpin(F->cr_mem_pref, QStringLiteral("memory_preferences_max_chars"));
    setSpin(F->cr_mem_tasks, QStringLiteral("memory_tasks_max_chars"));
    setSpin(F->cr_mem_avoid, QStringLiteral("memory_avoid_max_chars"));
    setSpin(F->cr_mem_facts, QStringLiteral("memory_facts_max_chars"));
}

} // namespace

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("设置"));
    resize(640, 520);

    PetConfig* cfg = PetConfig::getInstance();
    m_configPath = cfg->getConfigFilePath();

    auto* rootLayout = new QVBoxLayout(this);
    auto* pathLab = new QLabel(QStringLiteral("配置文件：%1").arg(m_configPath.isEmpty() ? QStringLiteral("（未知）") : m_configPath), this);
    pathLab->setWordWrap(true);
    pathLab->setStyleSheet(QStringLiteral("color: #555; font-size: 11px;"));
    rootLayout->addWidget(pathLab);

    if (!m_configPath.isEmpty()) {
        const QString tmplPath = QFileInfo(m_configPath).absolutePath()
                                 + QStringLiteral("/pet_config.defaults.json");
        auto* tmplLab = new QLabel(QStringLiteral("内置默认模板（恢复默认时从此复制）：%1").arg(tmplPath), this);
        tmplLab->setWordWrap(true);
        tmplLab->setStyleSheet(QStringLiteral("color: #666; font-size: 11px;"));
        rootLayout->addWidget(tmplLab);
    }

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    m_categoryList = new QListWidget(splitter);
    m_categoryList->setFixedWidth(132);
    m_categoryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_categoryList->setIconSize(QSize(0, 0));
    m_categoryList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_categoryList->setSpacing(2);
    m_categoryList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_categoryList->setStyleSheet(
        QStringLiteral("QListWidget { background: #f5f5f5; border: 1px solid #ccc; border-radius: 4px; padding: 4px; font-size: 12px; }"
                       "QListWidget::item { padding: 8px 6px; border-radius: 3px; }"
                       "QListWidget::item:selected { background: #e3f2fd; color: #1565c0; font-weight: bold; }"
                       "QListWidget::item:hover { background: #eeeeee; }"));

    m_pageStack = new QStackedWidget(splitter);
    m_pageStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    splitter->addWidget(m_categoryList);
    splitter->addWidget(m_pageStack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({132, 480});

    connect(m_categoryList, &QListWidget::currentRowChanged, m_pageStack, &QStackedWidget::setCurrentIndex);

    rootLayout->addWidget(splitter, 1);

    auto* btnRow = new QHBoxLayout();
    auto* saveBtn = new QPushButton(QStringLiteral("保存并应用"), this);
    auto* reloadBtn = new QPushButton(QStringLiteral("放弃更改并重新加载"), this);
    auto* restoreBtn = new QPushButton(QStringLiteral("恢复默认配置"), this);
    restoreBtn->setToolTip(QStringLiteral(
        "用仓库自带的 pet_config.defaults.json 覆盖当前 pet_config.json（不含密钥；聊天默认走 Ollama 占位）。请先备份自行修改的内容。"));
    auto* closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(reloadBtn);
    btnRow->addWidget(restoreBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    rootLayout->addLayout(btnRow);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(reloadBtn, &QPushButton::clicked, this, &SettingsDialog::onReload);
    connect(restoreBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    buildTabs();
    reloadFromDisk();
}

void SettingsDialog::buildTabs()
{
    auto* F = new FormWidgets();

    auto addPage = [this](const QString& title, QWidget* w) {
        m_categoryList->addItem(title);
        m_pageStack->addWidget(w);
    };

    /* ---------- 属性与待机 ---------- */
    {
        auto* page = new QWidget(this);
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(makeSectionHint(page, QStringLiteral("下列「衰减」均为每分钟属性变化点数（待机指普通待机状态）。数值越大变化越快。")));
        auto* form = new QFormLayout();
        lay->addLayout(form);

        F->a_default_level = spinInt(page, 1, 9999);
        form->addRow(QStringLiteral("默认等级"), F->a_default_level);
        F->a_default_exp = spinInt(page, 0, 99999999);
        form->addRow(QStringLiteral("默认经验"), F->a_default_exp);
        F->a_default_hunger = spinInt(page, 0, 100);
        form->addRow(QStringLiteral("默认饱食度"), F->a_default_hunger);
        F->a_default_energy = spinInt(page, 0, 100);
        form->addRow(QStringLiteral("默认精力"), F->a_default_energy);
        F->a_default_mood = spinInt(page, 0, 100);
        form->addRow(QStringLiteral("默认心情"), F->a_default_mood);

        F->a_hunger_decay_idle = spinInt(page, 0, 99999);
        form->addRow(QStringLiteral("待机饱食度衰减"), F->a_hunger_decay_idle);
        F->a_energy_decay_idle = spinInt(page, 0, 99999);
        form->addRow(QStringLiteral("待机精力衰减"), F->a_energy_decay_idle);
        F->a_mood_decay_idle = spinInt(page, 0, 99999);
        form->addRow(QStringLiteral("待机心情衰减"), F->a_mood_decay_idle);

        F->a_hunger_decay_abnormal = spinInt(page, 0, 99999);
        form->addRow(QStringLiteral("异常待机饱食度衰减"), F->a_hunger_decay_abnormal);
        F->a_energy_decay_abnormal = spinInt(page, 0, 99999);
        form->addRow(QStringLiteral("异常待机精力衰减"), F->a_energy_decay_abnormal);
        F->a_mood_decay_abnormal = spinInt(page, 0, 99999);
        form->addRow(QStringLiteral("异常待机心情衰减"), F->a_mood_decay_abnormal);

        lay->addStretch();
        addPage(QStringLiteral("属性·待机"), page);
    }

    /* ---------- 阈值 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->t_abnormal = spinInt(page, 0, 100);
        form->addRow(QStringLiteral("进入异常状态阈值（单项属性低于此值）"), F->t_abnormal);
        F->t_auto_sleep_energy = spinInt(page, 0, 100);
        form->addRow(QStringLiteral("自动入睡精力阈值"), F->t_auto_sleep_energy);
        F->t_recovery = spinInt(page, 0, 100);
        form->addRow(QStringLiteral("脱离异常恢复阈值"), F->t_recovery);
        addPage(QStringLiteral("阈值"), page);
    }

    /* ---------- 睡眠 ---------- */
    {
        auto* page = new QWidget(this);
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(makeSectionHint(page, QStringLiteral("睡眠中每分钟属性变化。")));
        auto* form = new QFormLayout();
        lay->addLayout(form);
        F->s_energy_recovery_pm = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("睡眠精力恢复（每分钟）"), F->s_energy_recovery_pm);
        F->s_hunger_decay_pm = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("睡眠饱食度衰减（每分钟）"), F->s_hunger_decay_pm);
        lay->addStretch();
        addPage(QStringLiteral("睡眠"), page);
    }

    /* ---------- 玩耍 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->p_mood_inc = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("心情增加（每分钟）"), F->p_mood_inc);
        F->p_energy_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("精力衰减（每分钟）"), F->p_energy_dec);
        F->p_hunger_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("饱食度衰减（每分钟）"), F->p_hunger_dec);
        F->p_exp_gain = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("经验增加（每分钟）"), F->p_exp_gain);
        addPage(QStringLiteral("玩耍"), page);
    }

    /* ---------- 工作 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->w_energy_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("精力衰减（每分钟）"), F->w_energy_dec);
        F->w_hunger_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("饱食度衰减（每分钟）"), F->w_hunger_dec);
        F->w_mood_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("心情衰减（每分钟）"), F->w_mood_dec);
        F->w_exp_gain = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("经验增加（每分钟）"), F->w_exp_gain);
        F->w_coin_gain = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("金币增加（每分钟）"), F->w_coin_gain);
        addPage(QStringLiteral("工作"), page);
    }

    /* ---------- 学习 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->st_energy_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("精力衰减（每分钟）"), F->st_energy_dec);
        F->st_hunger_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("饱食度衰减（每分钟）"), F->st_hunger_dec);
        F->st_mood_dec = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("心情衰减（每分钟）"), F->st_mood_dec);
        F->st_exp_gain = spinInt(page, 0, 999999);
        form->addRow(QStringLiteral("经验增加（每分钟）"), F->st_exp_gain);
        addPage(QStringLiteral("学习"), page);
    }

    /* ---------- 对话结算 ---------- */
    {
        auto* page = new QWidget(this);
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(makeSectionHint(page, QStringLiteral("每次 AI 对话成功结束时的单次结算（非每分钟）。")));
        auto* form = new QFormLayout();
        lay->addLayout(form);
        F->c_energy_cost = spinInt(page, 0, 999);
        form->addRow(QStringLiteral("精力消耗"), F->c_energy_cost);
        F->c_hunger_cost = spinInt(page, 0, 999);
        form->addRow(QStringLiteral("饱食消耗"), F->c_hunger_cost);
        F->c_mood_gain = spinInt(page, 0, 999);
        form->addRow(QStringLiteral("心情增加"), F->c_mood_gain);
        lay->addStretch();
        addPage(QStringLiteral("对话结算"), page);
    }

    /* ---------- 成长 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->g_exp_base = spinInt(page, 1, 999999);
        form->addRow(QStringLiteral("升级所需基础经验"), F->g_exp_base);
        F->g_exp_growth = spinInt(page, 0, 99999);
        form->addRow(QStringLiteral("每级额外经验增量"), F->g_exp_growth);
        addPage(QStringLiteral("成长"), page);
    }

    /* ---------- 聊天服务 ---------- */
    {
        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        auto* page = new QWidget(this);
        scroll->setWidget(page);
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(makeSectionHint(page, QStringLiteral("对接本地 Ollama 或兼容 OpenAI 的 HTTP API；修改后下次对话生效。"
                                                             " 云端地址可直接粘贴厂商给的 Base URL（含 https://…/v1）。")));

        lay->addWidget(makeSectionHint(page, QStringLiteral(
            "不清楚「上下文轮数、摘要」等如何填写时：先在下拉里选档位，再点「应用预设到表单」。"
            "只会填入与上下文/摘要相关的数字，不会改动提供方、模型名与 API Key。"
            "预设说明：初级≈本机 Llama 3.1 8B；中级≈ Qwen2.5-32B-Instruct；高级≈ DeepSeek V4-Flash。")));

        auto* presetBar = new QWidget(page);
        auto* presetLay = new QHBoxLayout(presetBar);
        presetLay->setContentsMargins(0, 0, 0, 0);
        F->cr_chat_preset = new QComboBox(page);
        populateChatRuntimePresetCombo(F->cr_chat_preset, m_configPath);
        F->cr_chat_preset_apply = new QPushButton(QStringLiteral("应用预设到表单"), page);
        F->cr_chat_preset_apply->setToolTip(QStringLiteral(
            "把所选档位的建议值写入下方对应输入框；仍需点「保存并应用」才会写入配置文件。"));
        presetLay->addWidget(F->cr_chat_preset, 1);
        presetLay->addWidget(F->cr_chat_preset_apply);
        lay->addWidget(presetBar);

        auto* form = new QFormLayout();
        lay->addLayout(form);

        F->cr_provider = new QComboBox(page);
        F->cr_provider->setEditable(true);
        F->cr_provider->addItems({QStringLiteral("ollama"), QStringLiteral("openai_compatible")});
        form->addRow(QStringLiteral("提供方"), F->cr_provider);

        F->cr_api_base = new QLineEdit(page);
        form->addRow(QStringLiteral("API 基础地址"), F->cr_api_base);

        F->cr_api_key = new QLineEdit(page);
        F->cr_api_key->setEchoMode(QLineEdit::Password);
        F->cr_api_key->setPlaceholderText(QStringLiteral("可选；填写则不必配置环境变量"));
        form->addRow(QStringLiteral("API Key"), F->cr_api_key);

        F->cr_api_key_env = new QLineEdit(page);
        F->cr_api_key_env->setPlaceholderText(QStringLiteral("可选；未填 Key 时从该环境变量读取"));
        form->addRow(QStringLiteral("API Key 环境变量名"), F->cr_api_key_env);

        F->cr_model = new QLineEdit(page);
        form->addRow(QStringLiteral("模型名"), F->cr_model);

        F->cr_script_path = new QLineEdit(page);
        form->addRow(QStringLiteral("对话脚本路径"), F->cr_script_path);

        F->cr_python_path = new QLineEdit(page);
        form->addRow(QStringLiteral("Python 命令"), F->cr_python_path);

        F->cr_ollama_host = new QLineEdit(page);
        form->addRow(QStringLiteral("Ollama 地址"), F->cr_ollama_host);

        F->cr_context_turns = spinInt(page, 1, 256);
        form->addRow(QStringLiteral("上下文轮数"), F->cr_context_turns);

        F->cr_max_context_chars = spinInt(page, 500, 9999999);
        form->addRow(QStringLiteral("上下文最大字符"), F->cr_max_context_chars);

        F->cr_max_reply_tokens = spinInt(page, 64, 8192);
        form->addRow(QStringLiteral("单次回复 token 上限"), F->cr_max_reply_tokens);

        F->cr_max_input_chars = spinInt(page, 50, 32000);
        form->addRow(QStringLiteral("单次输入最大字符"), F->cr_max_input_chars);

        F->cr_context_reserve = spinInt(page, 200, 50000);
        form->addRow(QStringLiteral("上下文预留字符(system+输入等)"), F->cr_context_reserve);

        F->cr_max_hist_est_tokens = spinInt(page, 0, 500000);
        form->addRow(QStringLiteral("历史估算token上限(0关闭)"), F->cr_max_hist_est_tokens);

        F->cr_chars_per_est_token = spinInt(page, 1, 16);
        form->addRow(QStringLiteral("估算token·每token字符数"), F->cr_chars_per_est_token);

        F->cr_summary_memory_chars = spinInt(page, 0, 500000);
        form->addRow(QStringLiteral("记忆总字符超则摘要(0仅轮数)"), F->cr_summary_memory_chars);

        F->cr_no_stage_directions = new QCheckBox(QStringLiteral("默认不使用括号舞台/动作描写"), page);
        form->addRow(QStringLiteral(""), F->cr_no_stage_directions);

        F->cr_pet_persona = new QPlainTextEdit(page);
        F->cr_pet_persona->setMinimumHeight(140);
        F->cr_pet_persona->setPlaceholderText(
            QStringLiteral("留空则使用脚本内置默认人设（与原版相同）。可在此自定义宠物性格与回复规则。"));
        form->addRow(QStringLiteral("宠物人设"), F->cr_pet_persona);

        F->cr_memory_path = new QLineEdit(page);
        form->addRow(QStringLiteral("记忆文件路径"), F->cr_memory_path);

        F->cr_summary_enabled = new QCheckBox(QStringLiteral("启用会话摘要"), page);
        form->addRow(QStringLiteral(""), F->cr_summary_enabled);

        F->cr_summary_compress_after = spinInt(page, 2, 9999);
        form->addRow(QStringLiteral("超过多少轮触发摘要"), F->cr_summary_compress_after);

        F->cr_summary_keep_recent = spinInt(page, 1, 999);
        form->addRow(QStringLiteral("摘要后保留最近轮数"), F->cr_summary_keep_recent);

        F->cr_summary_max_chars = spinInt(page, 50, 99999);
        form->addRow(QStringLiteral("摘要最大字符"), F->cr_summary_max_chars);

        F->cr_mem_pref = spinInt(page, 16, 99999);
        form->addRow(QStringLiteral("记忆槽·偏好 最大字符"), F->cr_mem_pref);
        F->cr_mem_tasks = spinInt(page, 16, 99999);
        form->addRow(QStringLiteral("记忆槽·任务 最大字符"), F->cr_mem_tasks);
        F->cr_mem_avoid = spinInt(page, 16, 99999);
        form->addRow(QStringLiteral("记忆槽·禁忌 最大字符"), F->cr_mem_avoid);
        F->cr_mem_facts = spinInt(page, 16, 99999);
        form->addRow(QStringLiteral("记忆槽·事实 最大字符"), F->cr_mem_facts);

        addPage(QStringLiteral("聊天服务"), scroll);
    }

    /* ---------- 聊天记忆（四槽，独立文件） ---------- */
    {
        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        auto* page = new QWidget(this);
        scroll->setWidget(page);
        auto* lay = new QVBoxLayout(page);
        lay->setContentsMargins(8, 8, 8, 8);
        lay->addWidget(makeSectionHint(page, QStringLiteral(
            "此处编辑 chat_memory.json 中的结构化四槽；文件路径由「聊天服务」页的「记忆文件路径」决定。"
            "修改后请按窗口底部「保存并应用」一并写入磁盘。")));

        F->cm_path = new QLabel(page);
        F->cm_path->setWordWrap(true);
        F->cm_path->setStyleSheet(QStringLiteral("color:#666;font-size:11px;"));
        lay->addWidget(F->cm_path);

        F->cm_msgs = new QLabel(page);
        F->cm_msgs->setStyleSheet(QStringLiteral("color:#666;font-size:11px;"));
        lay->addWidget(F->cm_msgs);

        lay->addWidget(makeSectionHint(page, QStringLiteral(
            "preferences=主人侧期望；facts=主人事实与稳定设定；禁忌=不宜话题；任务=约定/待办。")));

        auto* form = new QFormLayout();
        F->cm_pref = new QPlainTextEdit(page);
        F->cm_tasks = new QPlainTextEdit(page);
        F->cm_avoid = new QPlainTextEdit(page);
        F->cm_facts = new QPlainTextEdit(page);
        F->cm_pref->setMinimumHeight(64);
        F->cm_tasks->setMinimumHeight(64);
        F->cm_avoid->setMinimumHeight(48);
        F->cm_facts->setMinimumHeight(64);
        form->addRow(QStringLiteral("偏好 preferences（主人侧）"), F->cm_pref);
        form->addRow(QStringLiteral("任务 tasks"), F->cm_tasks);
        form->addRow(QStringLiteral("禁忌 avoid"), F->cm_avoid);
        form->addRow(QStringLiteral("事实 facts"), F->cm_facts);
        lay->addLayout(form);

        addPage(QStringLiteral("聊天记忆"), scroll);
    }

    /* ---------- 日志 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->log_verbose = new QCheckBox(QStringLiteral("输出详细状态日志（调试）"), page);
        form->addRow(QStringLiteral(""), F->log_verbose);
        addPage(QStringLiteral("日志"), page);
    }

    /* ---------- 移动 ---------- */
    {
        auto* page = new QWidget(this);
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(makeSectionHint(page, QStringLiteral("控制宠物在待机状态下无操作一段时间后的窗口随机游走。")));
        auto* form = new QFormLayout();
        lay->addLayout(form);
        F->m_idle_timeout = spinInt(page, 100, 9999999);
        form->addRow(QStringLiteral("无操作多久开始游走（毫秒）"), F->m_idle_timeout);
        F->m_move_speed = new QDoubleSpinBox(page);
        F->m_move_speed->setRange(0.05, 100.0);
        F->m_move_speed->setDecimals(2);
        F->m_move_speed->setSingleStep(0.1);
        form->addRow(QStringLiteral("移动速度（像素/帧）"), F->m_move_speed);
        F->m_update_interval = spinInt(page, 10, 500);
        form->addRow(QStringLiteral("位置刷新间隔（毫秒）"), F->m_update_interval);
        F->m_direction_keep = spinInt(page, 0, 100);
        form->addRow(QStringLiteral("保持原方向概率（%）"), F->m_direction_keep);
        lay->addStretch();
        addPage(QStringLiteral("移动"), page);
    }

    /* ---------- 互动 ---------- */
    {
        auto* page = new QWidget(this);
        auto* lay = new QVBoxLayout(page);
        lay->addWidget(makeSectionHint(page, QStringLiteral(
            "双击宠物生效：间隔内重复双击将被忽略（不增加心情、不播放双击动画）。设为 0 表示无间隔限制。"
            " 心情加成仅对「正常待机」「异常待机」生效，其它状态为 0。")));
        auto* form = new QFormLayout();
        lay->addLayout(form);
        F->i_dc_cooldown_ms = spinInt(page, 0, 9999999);
        form->addRow(QStringLiteral("双击最小间隔（毫秒）"), F->i_dc_cooldown_ms);
        F->i_dc_mood_idle = spinInt(page, -999, 999);
        form->addRow(QStringLiteral("正常待机·每次有效双击增加心情"), F->i_dc_mood_idle);
        F->i_dc_mood_abnormal = spinInt(page, -999, 999);
        form->addRow(QStringLiteral("异常待机·每次有效双击增加心情"), F->i_dc_mood_abnormal);
        lay->addStretch();
        addPage(QStringLiteral("互动"), page);
    }

    /* ---------- 窗口 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->win_width = spinInt(page, 50, 4000);
        form->addRow(QStringLiteral("窗口宽度（像素）"), F->win_width);
        F->win_height = spinInt(page, 50, 4000);
        form->addRow(QStringLiteral("窗口高度（像素）"), F->win_height);
        addPage(QStringLiteral("窗口"), page);
    }

    /* ---------- 动画缩放 ---------- */
    {
        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        auto* page = new QWidget(this);
        scroll->setWidget(page);
        auto* lay = new QVBoxLayout(page);
        lay->setContentsMargins(8, 8, 8, 8);
        lay->addWidget(makeSectionHint(page, QStringLiteral(
            "各 GIF 在宠物窗口内的相对大小（1.0 占满适配框）。保存并应用后立即生效。"
            "键与 resources/animations/ 下相对路径一致；其它 GIF 可在配置文件中 animation_scales 里手写键值。")));
        auto* form = new QFormLayout();
        lay->addLayout(form);
        for (const AnimScaleUiRow& row : kAnimScaleRows) {
            auto* sp = new QDoubleSpinBox(page);
            sp->setRange(0.05, 4.0);
            sp->setDecimals(2);
            sp->setSingleStep(0.05);
            sp->setValue(1.0);
            const QString key = QString::fromUtf8(row.relPath);
            F->anim_scale_spin.insert(key, sp);
            form->addRow(QStringLiteral("%1 (%2)")
                             .arg(QString::fromUtf8(row.labelZh))
                             .arg(key),
                         sp);
        }
        addPage(QStringLiteral("动画缩放"), scroll);
    }

    /* ---------- 属性上下限 ---------- */
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);
        F->l_max = spinInt(page, 1, 999999);
        form->addRow(QStringLiteral("属性上限"), F->l_max);
        F->l_min = spinInt(page, -999999, 999999);
        form->addRow(QStringLiteral("属性下限"), F->l_min);
        addPage(QStringLiteral("上下限"), page);
    }

    /* ---------- 食物 ---------- */
    {
        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        F->foodPage = new QWidget(this);
        scroll->setWidget(F->foodPage);
        F->foodOuterLayout = new QVBoxLayout(F->foodPage);
        F->foodOuterLayout->addWidget(makeSectionHint(F->foodPage, QStringLiteral("每种食物对应配置文件中的一个条目；保存后会写入磁盘。")));
        auto* foodListHost = new QWidget(F->foodPage);
        F->foodListLayout = new QVBoxLayout(foodListHost);
        F->foodListLayout->setContentsMargins(0, 0, 0, 0);
        F->foodOuterLayout->addWidget(foodListHost);
        F->foodOuterLayout->addStretch();
        addPage(QStringLiteral("食物"), scroll);
    }

    m_formWidgets = F;

    connect(F->cr_chat_preset_apply, &QPushButton::clicked, this, &SettingsDialog::onApplyChatRuntimePreset);

    if (m_categoryList->count() > 0) {
        m_categoryList->setCurrentRow(0);
    }
}

SettingsDialog::~SettingsDialog()
{
    if (m_formWidgets) {
        delete static_cast<FormWidgets*>(m_formWidgets);
        m_formWidgets = nullptr;
    }
}

void SettingsDialog::reloadFromDisk()
{
    if (!m_formWidgets)
        return;
    auto* F = static_cast<FormWidgets*>(m_formWidgets);

    if (m_configPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("设置"), QStringLiteral("未找到配置文件路径。"));
        return;
    }

    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("读取失败"), file.errorString());
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, QStringLiteral("JSON"), err.errorString());
        return;
    }

    applyRootJson(doc.object());
}

void SettingsDialog::applyRootJson(const QJsonObject& root)
{
    if (!m_formWidgets)
        return;
    auto* F = static_cast<FormWidgets*>(m_formWidgets);

    QJsonObject a = nested(root, QStringLiteral("attributes"));
    F->a_default_level->setValue(a.value(QStringLiteral("default_level")).toInt(1));
    F->a_default_exp->setValue(a.value(QStringLiteral("default_exp")).toInt(0));
    F->a_default_hunger->setValue(a.value(QStringLiteral("default_hunger")).toInt(50));
    F->a_default_energy->setValue(a.value(QStringLiteral("default_energy")).toInt(60));
    F->a_default_mood->setValue(a.value(QStringLiteral("default_mood")).toInt(70));
    F->a_hunger_decay_idle->setValue(a.value(QStringLiteral("hunger_decay_idle")).toInt(120));
    F->a_energy_decay_idle->setValue(a.value(QStringLiteral("energy_decay_idle")).toInt(120));
    F->a_mood_decay_idle->setValue(a.value(QStringLiteral("mood_decay_idle")).toInt(60));
    F->a_hunger_decay_abnormal->setValue(a.value(QStringLiteral("hunger_decay_abnormal")).toInt(60));
    F->a_energy_decay_abnormal->setValue(a.value(QStringLiteral("energy_decay_abnormal")).toInt(60));
    F->a_mood_decay_abnormal->setValue(a.value(QStringLiteral("mood_decay_abnormal")).toInt(120));

    QJsonObject t = nested(root, QStringLiteral("thresholds"));
    F->t_abnormal->setValue(t.value(QStringLiteral("abnormal")).toInt(20));
    F->t_auto_sleep_energy->setValue(t.value(QStringLiteral("auto_sleep_energy")).toInt(5));
    F->t_recovery->setValue(t.value(QStringLiteral("recovery")).toInt(20));

    QJsonObject sl = nested(root, QStringLiteral("sleep"));
    if (sl.contains(QStringLiteral("energy_recovery_per_minute"))) {
        F->s_energy_recovery_pm->setValue(sl.value(QStringLiteral("energy_recovery_per_minute")).toInt(480));
    } else {
        F->s_energy_recovery_pm->setValue(sl.value(QStringLiteral("energy_recovery")).toInt(8) * 60);
    }
    if (sl.contains(QStringLiteral("hunger_decay_per_minute"))) {
        F->s_hunger_decay_pm->setValue(sl.value(QStringLiteral("hunger_decay_per_minute")).toInt(60));
    } else {
        F->s_hunger_decay_pm->setValue(sl.value(QStringLiteral("hunger_decay")).toInt(1) * 60);
    }
    QJsonObject pl = nested(root, QStringLiteral("play"));
    if (pl.contains(QStringLiteral("mood_increase_per_minute"))) {
        F->p_mood_inc->setValue(pl.value(QStringLiteral("mood_increase_per_minute")).toInt(600));
    } else {
        F->p_mood_inc->setValue(pl.value(QStringLiteral("mood_increase")).toInt(10) * 60);
    }
    if (pl.contains(QStringLiteral("energy_decay_per_minute"))) {
        F->p_energy_dec->setValue(pl.value(QStringLiteral("energy_decay_per_minute")).toInt(180));
    } else {
        F->p_energy_dec->setValue(pl.value(QStringLiteral("energy_decay")).toInt(3) * 60);
    }
    if (pl.contains(QStringLiteral("hunger_decay_per_minute"))) {
        F->p_hunger_dec->setValue(pl.value(QStringLiteral("hunger_decay_per_minute")).toInt(120));
    } else {
        F->p_hunger_dec->setValue(pl.value(QStringLiteral("hunger_decay")).toInt(2) * 60);
    }
    if (pl.contains(QStringLiteral("exp_gain_per_minute"))) {
        F->p_exp_gain->setValue(pl.value(QStringLiteral("exp_gain_per_minute")).toInt(120));
    } else {
        F->p_exp_gain->setValue(pl.value(QStringLiteral("exp_gain")).toInt(2) * 60);
    }

    QJsonObject wo = nested(root, QStringLiteral("work"));
    if (wo.contains(QStringLiteral("energy_decay_per_minute"))) {
        F->w_energy_dec->setValue(wo.value(QStringLiteral("energy_decay_per_minute")).toInt(240));
    } else {
        F->w_energy_dec->setValue(wo.value(QStringLiteral("energy_decay")).toInt(4) * 60);
    }
    if (wo.contains(QStringLiteral("hunger_decay_per_minute"))) {
        F->w_hunger_dec->setValue(wo.value(QStringLiteral("hunger_decay_per_minute")).toInt(180));
    } else {
        F->w_hunger_dec->setValue(wo.value(QStringLiteral("hunger_decay")).toInt(3) * 60);
    }
    if (wo.contains(QStringLiteral("mood_decay_per_minute"))) {
        F->w_mood_dec->setValue(wo.value(QStringLiteral("mood_decay_per_minute")).toInt(120));
    } else {
        F->w_mood_dec->setValue(wo.value(QStringLiteral("mood_decay")).toInt(2) * 60);
    }
    if (wo.contains(QStringLiteral("exp_gain_per_minute"))) {
        F->w_exp_gain->setValue(wo.value(QStringLiteral("exp_gain_per_minute")).toInt(180));
    } else {
        F->w_exp_gain->setValue(wo.value(QStringLiteral("exp_gain")).toInt(3) * 60);
    }
    if (wo.contains(QStringLiteral("coin_gain_per_minute"))) {
        F->w_coin_gain->setValue(wo.value(QStringLiteral("coin_gain_per_minute")).toInt(300));
    } else {
        F->w_coin_gain->setValue(wo.value(QStringLiteral("coin_gain")).toInt(5) * 60);
    }

    QJsonObject st = nested(root, QStringLiteral("study"));
    if (st.contains(QStringLiteral("energy_decay_per_minute"))) {
        F->st_energy_dec->setValue(st.value(QStringLiteral("energy_decay_per_minute")).toInt(300));
    } else {
        F->st_energy_dec->setValue(st.value(QStringLiteral("energy_decay")).toInt(5) * 60);
    }
    if (st.contains(QStringLiteral("hunger_decay_per_minute"))) {
        F->st_hunger_dec->setValue(st.value(QStringLiteral("hunger_decay_per_minute")).toInt(120));
    } else {
        F->st_hunger_dec->setValue(st.value(QStringLiteral("hunger_decay")).toInt(2) * 60);
    }
    if (st.contains(QStringLiteral("mood_decay_per_minute"))) {
        F->st_mood_dec->setValue(st.value(QStringLiteral("mood_decay_per_minute")).toInt(60));
    } else {
        F->st_mood_dec->setValue(st.value(QStringLiteral("mood_decay")).toInt(1) * 60);
    }
    if (st.contains(QStringLiteral("exp_gain_per_minute"))) {
        F->st_exp_gain->setValue(st.value(QStringLiteral("exp_gain_per_minute")).toInt(300));
    } else {
        F->st_exp_gain->setValue(st.value(QStringLiteral("exp_gain")).toInt(5) * 60);
    }

    QJsonObject ch = nested(root, QStringLiteral("chat"));
    F->c_energy_cost->setValue(ch.value(QStringLiteral("energy_cost_per_round")).toInt(ch.value(QStringLiteral("energy_decay")).toInt(1)));
    F->c_hunger_cost->setValue(ch.value(QStringLiteral("hunger_cost_per_round")).toInt(ch.value(QStringLiteral("hunger_decay")).toInt(1)));
    F->c_mood_gain->setValue(ch.value(QStringLiteral("mood_gain_per_round")).toInt(ch.value(QStringLiteral("mood_increase")).toInt(2)));

    QJsonObject pg = nested(root, QStringLiteral("progression"));
    F->g_exp_base->setValue(pg.value(QStringLiteral("exp_base")).toInt(100));
    F->g_exp_growth->setValue(pg.value(QStringLiteral("exp_growth")).toInt(20));

    QJsonObject cr = nested(root, QStringLiteral("chat_runtime"));
    {
        QString prov = cr.value(QStringLiteral("provider")).toString(QStringLiteral("ollama")).trimmed();
        int idx = F->cr_provider->findText(prov);
        if (idx >= 0) {
            F->cr_provider->setCurrentIndex(idx);
        } else {
            F->cr_provider->setEditText(prov);
        }
        F->cr_api_base->setText(cr.value(QStringLiteral("api_base")).toString());
        F->cr_api_key->setText(cr.value(QStringLiteral("api_key")).toString());
        F->cr_api_key_env->setText(cr.value(QStringLiteral("api_key_env")).toString());
        F->cr_model->setText(cr.value(QStringLiteral("model")).toString(QStringLiteral("llama3.1:8b")));
        F->cr_script_path->setText(cr.value(QStringLiteral("script_path")).toString(QStringLiteral("resources/scripts/chat_ai.py")));
        F->cr_python_path->setText(cr.value(QStringLiteral("python_path")).toString(QStringLiteral("python3")));
        F->cr_ollama_host->setText(cr.value(QStringLiteral("ollama_host")).toString(QStringLiteral("http://127.0.0.1:11434")));
        F->cr_context_turns->setValue(cr.value(QStringLiteral("context_turns")).toInt(8));
        F->cr_max_context_chars->setValue(cr.value(QStringLiteral("max_context_chars")).toInt(8000));
        F->cr_max_reply_tokens->setValue(cr.value(QStringLiteral("max_reply_tokens")).toInt(1024));
        F->cr_max_input_chars->setValue(cr.value(QStringLiteral("max_input_chars")).toInt(800));
        F->cr_context_reserve->setValue(cr.value(QStringLiteral("context_reserve_chars")).toInt(2000));
        F->cr_max_hist_est_tokens->setValue(cr.value(QStringLiteral("max_history_estimated_tokens")).toInt(0));
        F->cr_chars_per_est_token->setValue(cr.value(QStringLiteral("context_chars_per_est_token")).toInt(3));
        F->cr_summary_memory_chars->setValue(cr.value(QStringLiteral("summary_compress_after_memory_chars")).toInt(2800));
        F->cr_no_stage_directions->setChecked(cr.value(QStringLiteral("reply_no_stage_directions")).toBool(true));
        F->cr_pet_persona->setPlainText(cr.value(QStringLiteral("pet_persona")).toString());
        F->cr_memory_path->setText(cr.value(QStringLiteral("memory_path")).toString(QStringLiteral("resources/save/chat_memory.json")));
        F->cr_summary_enabled->setChecked(cr.value(QStringLiteral("summary_enabled")).toBool(true));
        F->cr_summary_compress_after->setValue(cr.value(QStringLiteral("summary_compress_after_turns")).toInt(12));
        F->cr_summary_keep_recent->setValue(cr.value(QStringLiteral("summary_keep_recent_turns")).toInt(8));
        F->cr_summary_max_chars->setValue(cr.value(QStringLiteral("summary_max_chars")).toInt(400));
        const int quarter = qMax(32, F->cr_summary_max_chars->value() / 4);
        F->cr_mem_pref->setValue(cr.value(QStringLiteral("memory_preferences_max_chars")).toInt(quarter));
        F->cr_mem_tasks->setValue(cr.value(QStringLiteral("memory_tasks_max_chars")).toInt(quarter));
        F->cr_mem_avoid->setValue(cr.value(QStringLiteral("memory_avoid_max_chars")).toInt(qMax(24, quarter - 20)));
        F->cr_mem_facts->setValue(cr.value(QStringLiteral("memory_facts_max_chars")).toInt(quarter));
    }

    QJsonObject lg = nested(root, QStringLiteral("logging"));
    F->log_verbose->setChecked(lg.value(QStringLiteral("verbose_state_logs")).toBool(false));

    QJsonObject mv = nested(root, QStringLiteral("movement"));
    F->m_idle_timeout->setValue(mv.value(QStringLiteral("idle_timeout")).toInt(5000));
    F->m_move_speed->setValue(mv.value(QStringLiteral("move_speed")).toDouble(2.0));
    F->m_update_interval->setValue(mv.value(QStringLiteral("update_interval")).toInt(33));
    F->m_direction_keep->setValue(mv.value(QStringLiteral("direction_keep_rate")).toInt(80));

    QJsonObject inter = nested(root, QStringLiteral("interaction"));
    F->i_dc_cooldown_ms->setValue(inter.value(QStringLiteral("double_click_cooldown_ms")).toInt(10000));
    F->i_dc_mood_idle->setValue(inter.value(QStringLiteral("double_click_mood_delta_idle")).toInt(2));
    F->i_dc_mood_abnormal->setValue(inter.value(QStringLiteral("double_click_mood_delta_abnormal")).toInt(1));

    QJsonObject win = nested(root, QStringLiteral("window"));
    F->win_width->setValue(win.value(QStringLiteral("width")).toInt(300));
    F->win_height->setValue(win.value(QStringLiteral("height")).toInt(300));

    {
        QJsonObject asc;
        if (root.contains(QStringLiteral("animation_scales")) && root.value(QStringLiteral("animation_scales")).isObject()) {
            asc = root.value(QStringLiteral("animation_scales")).toObject();
        }
        for (auto it = F->anim_scale_spin.constBegin(); it != F->anim_scale_spin.constEnd(); ++it) {
            if (it.value())
                it.value()->setValue(asc.value(it.key()).toDouble(1.0));
        }
    }

    QJsonObject lim = nested(root, QStringLiteral("limits"));
    F->l_max->setValue(lim.value(QStringLiteral("max_value")).toInt(100));
    F->l_min->setValue(lim.value(QStringLiteral("min_value")).toInt(0));

    clearFoodRows(F);

    QJsonObject foodsObj = nested(root, QStringLiteral("foods"));
    QStringList keys = foodsObj.keys();
    keys.sort();
    for (const QString& key : keys) {
        if (!foodsObj.value(key).isObject())
            continue;
        appendFoodRow(F, key, foodsObj.value(key).toObject());
    }

    loadChatMemoryIntoForm(F);
}

QJsonObject SettingsDialog::collectRootJson() const
{
    if (!m_formWidgets)
        return QJsonObject();
    const auto* F = static_cast<const FormWidgets*>(m_formWidgets);

    QJsonObject root;

    QJsonObject a;
    a.insert(QStringLiteral("default_level"), F->a_default_level->value());
    a.insert(QStringLiteral("default_exp"), F->a_default_exp->value());
    a.insert(QStringLiteral("default_hunger"), F->a_default_hunger->value());
    a.insert(QStringLiteral("default_energy"), F->a_default_energy->value());
    a.insert(QStringLiteral("default_mood"), F->a_default_mood->value());
    a.insert(QStringLiteral("hunger_decay_idle"), F->a_hunger_decay_idle->value());
    a.insert(QStringLiteral("energy_decay_idle"), F->a_energy_decay_idle->value());
    a.insert(QStringLiteral("mood_decay_idle"), F->a_mood_decay_idle->value());
    a.insert(QStringLiteral("hunger_decay_abnormal"), F->a_hunger_decay_abnormal->value());
    a.insert(QStringLiteral("energy_decay_abnormal"), F->a_energy_decay_abnormal->value());
    a.insert(QStringLiteral("mood_decay_abnormal"), F->a_mood_decay_abnormal->value());
    root.insert(QStringLiteral("attributes"), a);

    QJsonObject t;
    t.insert(QStringLiteral("abnormal"), F->t_abnormal->value());
    t.insert(QStringLiteral("auto_sleep_energy"), F->t_auto_sleep_energy->value());
    t.insert(QStringLiteral("recovery"), F->t_recovery->value());
    root.insert(QStringLiteral("thresholds"), t);

    QJsonObject sl;
    sl.insert(QStringLiteral("energy_recovery_per_minute"), F->s_energy_recovery_pm->value());
    sl.insert(QStringLiteral("hunger_decay_per_minute"), F->s_hunger_decay_pm->value());
    root.insert(QStringLiteral("sleep"), sl);

    QJsonObject pl;
    pl.insert(QStringLiteral("mood_increase_per_minute"), F->p_mood_inc->value());
    pl.insert(QStringLiteral("energy_decay_per_minute"), F->p_energy_dec->value());
    pl.insert(QStringLiteral("hunger_decay_per_minute"), F->p_hunger_dec->value());
    pl.insert(QStringLiteral("exp_gain_per_minute"), F->p_exp_gain->value());
    root.insert(QStringLiteral("play"), pl);

    QJsonObject wo;
    wo.insert(QStringLiteral("energy_decay_per_minute"), F->w_energy_dec->value());
    wo.insert(QStringLiteral("hunger_decay_per_minute"), F->w_hunger_dec->value());
    wo.insert(QStringLiteral("mood_decay_per_minute"), F->w_mood_dec->value());
    wo.insert(QStringLiteral("exp_gain_per_minute"), F->w_exp_gain->value());
    wo.insert(QStringLiteral("coin_gain_per_minute"), F->w_coin_gain->value());
    root.insert(QStringLiteral("work"), wo);

    QJsonObject st;
    st.insert(QStringLiteral("energy_decay_per_minute"), F->st_energy_dec->value());
    st.insert(QStringLiteral("hunger_decay_per_minute"), F->st_hunger_dec->value());
    st.insert(QStringLiteral("mood_decay_per_minute"), F->st_mood_dec->value());
    st.insert(QStringLiteral("exp_gain_per_minute"), F->st_exp_gain->value());
    root.insert(QStringLiteral("study"), st);

    QJsonObject ch;
    ch.insert(QStringLiteral("energy_cost_per_round"), F->c_energy_cost->value());
    ch.insert(QStringLiteral("hunger_cost_per_round"), F->c_hunger_cost->value());
    ch.insert(QStringLiteral("mood_gain_per_round"), F->c_mood_gain->value());
    root.insert(QStringLiteral("chat"), ch);

    QJsonObject pg;
    pg.insert(QStringLiteral("exp_base"), F->g_exp_base->value());
    pg.insert(QStringLiteral("exp_growth"), F->g_exp_growth->value());
    root.insert(QStringLiteral("progression"), pg);

    QJsonObject cr;
    cr.insert(QStringLiteral("provider"), F->cr_provider->currentText().trimmed());
    cr.insert(QStringLiteral("api_base"), F->cr_api_base->text());
    cr.insert(QStringLiteral("api_key"), F->cr_api_key->text());
    cr.insert(QStringLiteral("api_key_env"), F->cr_api_key_env->text());
    cr.insert(QStringLiteral("model"), F->cr_model->text());
    cr.insert(QStringLiteral("script_path"), F->cr_script_path->text());
    cr.insert(QStringLiteral("python_path"), F->cr_python_path->text());
    cr.insert(QStringLiteral("ollama_host"), F->cr_ollama_host->text());
    cr.insert(QStringLiteral("context_turns"), F->cr_context_turns->value());
    cr.insert(QStringLiteral("max_context_chars"), F->cr_max_context_chars->value());
    cr.insert(QStringLiteral("max_reply_tokens"), F->cr_max_reply_tokens->value());
    cr.insert(QStringLiteral("max_input_chars"), F->cr_max_input_chars->value());
    cr.insert(QStringLiteral("context_reserve_chars"), F->cr_context_reserve->value());
    cr.insert(QStringLiteral("max_history_estimated_tokens"), F->cr_max_hist_est_tokens->value());
    cr.insert(QStringLiteral("context_chars_per_est_token"), F->cr_chars_per_est_token->value());
    cr.insert(QStringLiteral("summary_compress_after_memory_chars"), F->cr_summary_memory_chars->value());
    cr.insert(QStringLiteral("reply_no_stage_directions"), F->cr_no_stage_directions->isChecked());
    cr.insert(QStringLiteral("pet_persona"), F->cr_pet_persona->toPlainText());
    cr.insert(QStringLiteral("memory_path"), F->cr_memory_path->text());
    cr.insert(QStringLiteral("summary_enabled"), F->cr_summary_enabled->isChecked());
    cr.insert(QStringLiteral("summary_compress_after_turns"), F->cr_summary_compress_after->value());
    cr.insert(QStringLiteral("summary_keep_recent_turns"), F->cr_summary_keep_recent->value());
    cr.insert(QStringLiteral("summary_max_chars"), F->cr_summary_max_chars->value());
    cr.insert(QStringLiteral("memory_preferences_max_chars"), F->cr_mem_pref->value());
    cr.insert(QStringLiteral("memory_tasks_max_chars"), F->cr_mem_tasks->value());
    cr.insert(QStringLiteral("memory_avoid_max_chars"), F->cr_mem_avoid->value());
    cr.insert(QStringLiteral("memory_facts_max_chars"), F->cr_mem_facts->value());
    root.insert(QStringLiteral("chat_runtime"), cr);

    QJsonObject lg;
    lg.insert(QStringLiteral("verbose_state_logs"), F->log_verbose->isChecked());
    root.insert(QStringLiteral("logging"), lg);

    QJsonObject mv;
    mv.insert(QStringLiteral("idle_timeout"), F->m_idle_timeout->value());
    mv.insert(QStringLiteral("move_speed"), F->m_move_speed->value());
    mv.insert(QStringLiteral("update_interval"), F->m_update_interval->value());
    mv.insert(QStringLiteral("direction_keep_rate"), F->m_direction_keep->value());
    root.insert(QStringLiteral("movement"), mv);

    QJsonObject inter;
    inter.insert(QStringLiteral("double_click_cooldown_ms"), F->i_dc_cooldown_ms->value());
    inter.insert(QStringLiteral("double_click_mood_delta_idle"), F->i_dc_mood_idle->value());
    inter.insert(QStringLiteral("double_click_mood_delta_abnormal"), F->i_dc_mood_abnormal->value());
    root.insert(QStringLiteral("interaction"), inter);

    QJsonObject win;
    win.insert(QStringLiteral("width"), F->win_width->value());
    win.insert(QStringLiteral("height"), F->win_height->value());
    root.insert(QStringLiteral("window"), win);

    QJsonObject asc;
    for (auto it = F->anim_scale_spin.constBegin(); it != F->anim_scale_spin.constEnd(); ++it) {
        if (it.value())
            asc.insert(it.key(), it.value()->value());
    }
    root.insert(QStringLiteral("animation_scales"), asc);

    QJsonObject lim;
    lim.insert(QStringLiteral("max_value"), F->l_max->value());
    lim.insert(QStringLiteral("min_value"), F->l_min->value());
    root.insert(QStringLiteral("limits"), lim);

    QJsonObject foods;
    for (const FoodRowWidgets& row : F->foodRows) {
        QJsonObject o;
        o.insert(QStringLiteral("name"), row.name->text());
        o.insert(QStringLiteral("price"), row.price->value());
        o.insert(QStringLiteral("hunger"), row.hunger->value());
        o.insert(QStringLiteral("energy"), row.energy->value());
        o.insert(QStringLiteral("mood"), row.mood->value());
        o.insert(QStringLiteral("exp"), row.exp->value());
        foods.insert(row.id, o);
    }
    root.insert(QStringLiteral("foods"), foods);

    return root;
}

void SettingsDialog::onApplyChatRuntimePreset()
{
    if (!m_formWidgets) {
        return;
    }
    auto* F = static_cast<FormWidgets*>(m_formWidgets);
    if (!F->cr_chat_preset || F->cr_chat_preset->currentIndex() <= 0) {
        QMessageBox::information(this, QStringLiteral("档位预设"),
                                 QStringLiteral("请先在左侧下拉框中选择一个档位（非「选择档位」首项）。"));
        return;
    }
    const QVariantMap m = F->cr_chat_preset->currentData().toMap();
    if (m.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("档位预设"), QStringLiteral("当前选项没有可用的预设数据。"));
        return;
    }
    applyChatRuntimePresetMap(F, m);
    QMessageBox::information(this, QStringLiteral("档位预设"),
                             QStringLiteral("已写入表单中的上下文与摘要相关数值。"
                                            "若满意请点击窗口底部的「保存并应用」才会保存到配置文件。"));
}

void SettingsDialog::onReload()
{
    reloadFromDisk();
}

void SettingsDialog::onSave()
{
    PetConfig* cfg = PetConfig::getInstance();
    if (m_configPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("设置"), QStringLiteral("没有可用的配置文件路径。"));
        return;
    }

    QJsonObject root = collectRootJson();
    QJsonDocument doc(root);

    QFile out(m_configPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, QStringLiteral("写入失败"), out.errorString());
        return;
    }

    const QByteArray pretty = doc.toJson(QJsonDocument::Indented);
    if (out.write(pretty) != pretty.size()) {
        out.close();
        QMessageBox::critical(this, QStringLiteral("写入失败"), QStringLiteral("写入不完整。"));
        return;
    }
    out.close();

    if (!cfg->loadConfig(m_configPath)) {
        QMessageBox::critical(this, QStringLiteral("应用失败"),
                              QStringLiteral("文件已保存，但加载到内存失败，请检查数值范围是否合理。"));
        return;
    }

    auto* F = static_cast<FormWidgets*>(m_formWidgets);
    if (saveChatMemoryFromForm(F)) {
        emit chatMemoryEdited();
    }

    QMessageBox::information(this, QStringLiteral("设置"), QStringLiteral("已保存并热更新生效。"));
}

QString SettingsDialog::resolveDefaultsTemplatePath() const
{
    if (m_configPath.isEmpty())
        return {};
    return QFileInfo(m_configPath).absolutePath() + QStringLiteral("/pet_config.defaults.json");
}

void SettingsDialog::onRestoreDefaults()
{
    PetConfig* cfg = PetConfig::getInstance();
    if (m_configPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("设置"), QStringLiteral("没有可用的配置文件路径。"));
        return;
    }

    const QString tmplPath = resolveDefaultsTemplatePath();
    if (!QFile::exists(tmplPath)) {
        QMessageBox::critical(this, QStringLiteral("恢复默认"),
                              QStringLiteral("未找到默认模板文件：\n%1").arg(tmplPath));
        return;
    }

    const auto confirm = QMessageBox::question(
        this,
        QStringLiteral("恢复默认配置"),
        QStringLiteral("将用内置模板覆盖当前配置文件（建议先自行备份）：\n\n%1\n\n确定继续吗？")
            .arg(m_configPath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (confirm != QMessageBox::Yes)
        return;

    QFile src(tmplPath);
    if (!src.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QStringLiteral("读取失败"), src.errorString());
        return;
    }
    const QByteArray data = src.readAll();
    src.close();

    QFile dst(m_configPath);
    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, QStringLiteral("写入失败"), dst.errorString());
        return;
    }
    if (dst.write(data) != static_cast<qint64>(data.size())) {
        dst.close();
        QMessageBox::critical(this, QStringLiteral("写入失败"), QStringLiteral("写入不完整。"));
        return;
    }
    dst.close();

    if (!cfg->loadConfig(m_configPath)) {
        QMessageBox::critical(this, QStringLiteral("应用失败"),
                              QStringLiteral("已写入默认内容，但加载到内存失败，请检查 JSON。"));
        reloadFromDisk();
        return;
    }

    reloadFromDisk();
    QMessageBox::information(this, QStringLiteral("设置"), QStringLiteral("已恢复为默认配置并热更新。"));
}
