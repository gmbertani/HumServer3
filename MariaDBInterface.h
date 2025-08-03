#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

class MariaDBInterface : public QObject
{
    Q_OBJECT

public:
    explicit MariaDBInterface(QObject *parent = nullptr);

    bool connect(const QString &host, int port, const QString &user, const QString &password);
    bool ensureDatabaseAndTables();

    // Placeholder method for future query
    void exampleQuery();

private:
    QSqlDatabase db;
    QString dbName = "humDB";

    bool databaseExists();
    bool createDatabaseAndTables();
};