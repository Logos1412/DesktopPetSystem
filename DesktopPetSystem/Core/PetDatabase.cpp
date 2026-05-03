#pragma execution_character_set("utf-8")

#include "PetDatabase.h"
#include "PetAttribute.h"
#include <QDebug>
#include <QFileInfo>

PetDatabase::PetDatabase(QObject* parent)
    : QObject(parent)
{
}

PetDatabase::~PetDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool PetDatabase::initDatabase(const QString& dbPath)
{
    m_dbPath = dbPath;

    if (QSqlDatabase::contains("petdb")) {
        m_db = QSqlDatabase::database("petdb");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "petdb");
    }
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "[数据库] 打开失败:" << m_db.lastError().text();
        return false;
    }

    if (!createTables()) {
        qWarning() << "[数据库] 创建表结构失败";
        return false;
    }

    qDebug() << "[数据库] 初始化成功:" << dbPath;
    return true;
}

bool PetDatabase::createTables()
{
    QSqlQuery query(m_db);

    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS pet_attribute (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            level INTEGER NOT NULL DEFAULT 1,
            exp INTEGER NOT NULL DEFAULT 0,
            hunger INTEGER NOT NULL DEFAULT 50,
            energy INTEGER NOT NULL DEFAULT 60,
            mood INTEGER NOT NULL DEFAULT 70,
            coin INTEGER NOT NULL DEFAULT 0,
            update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createTableSql)) {
        qWarning() << "[数据库] 创建表失败:" << query.lastError().text();
        return false;
    }

    if (!query.exec("INSERT OR IGNORE INTO pet_attribute (id) VALUES (1)")) {
        qWarning() << "[数据库] 初始化默认数据失败:" << query.lastError().text();
        return false;
    }

    return true;
}

bool PetDatabase::saveAttribute(const PetAttribute* attr)
{
    if (!m_db.isOpen()) {
        return false;
    }

    QSqlQuery query(m_db);
    m_db.transaction();

    QString updateSql = R"(
        UPDATE pet_attribute SET 
            level = :level, 
            exp = :exp, 
            hunger = :hunger, 
            energy = :energy, 
            mood = :mood, 
            coin = :coin, 
            update_time = CURRENT_TIMESTAMP 
        WHERE id = 1
    )";

    query.prepare(updateSql);
    query.bindValue(":level", attr->getLevel());
    query.bindValue(":exp", attr->getExp());
    query.bindValue(":hunger", attr->getHunger());
    query.bindValue(":energy", attr->getEnergy());
    query.bindValue(":mood", attr->getMood());
    query.bindValue(":coin", attr->getCoin());

    if (!query.exec()) {
        qWarning() << "[数据库] 保存属性失败:" << query.lastError().text();
        m_db.rollback();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        QString insertSql = R"(
            INSERT INTO pet_attribute (level, exp, hunger, energy, mood, coin)
            VALUES (:level, :exp, :hunger, :energy, :mood, :coin)
        )";

        query.prepare(insertSql);
        query.bindValue(":level", attr->getLevel());
        query.bindValue(":exp", attr->getExp());
        query.bindValue(":hunger", attr->getHunger());
        query.bindValue(":energy", attr->getEnergy());
        query.bindValue(":mood", attr->getMood());
        query.bindValue(":coin", attr->getCoin());

        if (!query.exec()) {
            qWarning() << "[数据库] 插入属性失败:" << query.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        qWarning() << "[数据库] 提交事务失败:" << m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}

bool PetDatabase::loadAttribute(PetAttribute* attr)
{
    if (!m_db.isOpen()) {
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT level, exp, hunger, energy, mood, coin FROM pet_attribute WHERE id = 1")) {
        qWarning() << "[数据库] 加载属性失败:" << query.lastError().text();
        return false;
    }

    if (!query.next()) {
        return false;
    }

    attr->m_level = query.value("level").toInt();
    attr->m_exp = query.value("exp").toInt();
    attr->m_hunger = query.value("hunger").toInt();
    attr->m_energy = query.value("energy").toInt();
    attr->m_mood = query.value("mood").toInt();
    attr->m_coin = query.value("coin").toInt();

    return true;
}

bool PetDatabase::databaseExists() const
{
    QFileInfo fileInfo(m_dbPath);
    return fileInfo.exists() && fileInfo.isFile();
}
