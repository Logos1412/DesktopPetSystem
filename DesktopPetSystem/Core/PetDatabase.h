#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class PetAttribute;

class PetDatabase : public QObject
{
    Q_OBJECT

public:
    explicit PetDatabase(QObject* parent = nullptr);
    ~PetDatabase() override;

    bool initDatabase(const QString& dbPath);
    bool saveAttribute(const PetAttribute* attr);
    bool loadAttribute(PetAttribute* attr);
    bool databaseExists() const;

private:
    bool createTables();

private:
    QSqlDatabase m_db;
    QString m_dbPath;
};
