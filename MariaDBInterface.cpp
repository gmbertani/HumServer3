#include "MariaDBInterface.h"
#include <QDebug>

MariaDBInterface::MariaDBInterface(QObject *parent)
    : QObject(parent)
{
}

bool MariaDBInterface::connect(const QString &host, int port, const QString &user, const QString &password)
{
    db = QSqlDatabase::addDatabase("QMYSQL");

    db.setHostName(host);
    db.setPort(port);
    db.setUserName(user);
    db.setPassword(password);
    db.setDatabaseName("mysql"); // Temporarily use system DB for checking

    if (!db.open())
    {
        qCritical() << "Failed to connect to MariaDB:" << db.lastError().text();
        return false;
    }

    return true;
}

bool MariaDBInterface::databaseExists()
{
    QSqlQuery query("SHOW DATABASES LIKE 'humDB'");
    return query.next();
}

bool MariaDBInterface::ensureDatabaseAndTables()
{
    if (databaseExists())
    {
        qDebug() << "Database 'humDB' already exists.";
        return true;
    }

    return createDatabaseAndTables();
}

bool MariaDBInterface::createDatabaseAndTables()
{
    QStringList sqlStatements = {
        "DROP DATABASE IF EXISTS humDB;",
        "CREATE DATABASE humDB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;",
        "USE humDB;",
        "CREATE TABLE t_patients ("
        "  ID INT AUTO_INCREMENT PRIMARY KEY,"
        "  name VARCHAR(255) NOT NULL,"
        "  surname VARCHAR(255) NOT NULL,"
        "  height_cm INT,"
        "  weight_kg INT"
        ");",
        "CREATE TABLE t_types ("
        "  ID INT AUTO_INCREMENT PRIMARY KEY,"
        "  exa_type VARCHAR(255) NOT NULL UNIQUE"
        ");",
        "CREATE TABLE t_exams ("
        "  ID INT AUTO_INCREMENT PRIMARY KEY,"
        "  IDexa INT NOT NULL,"
        "  IDpatient INT NOT NULL,"
        "  date DATE NOT NULL,"
        "  time TIME NOT NULL,"
        "  frames BLOB,"
        "  FOREIGN KEY (IDexa) REFERENCES t_types(ID),"
        "  FOREIGN KEY (IDpatient) REFERENCES t_patients(ID)"
        ");"
    };

    QSqlQuery query;
    for (const QString &stmt : sqlStatements)
    {
        if (!query.exec(stmt))
        {
            qCritical() << "SQL error:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

void MariaDBInterface::exampleQuery()
{
    qDebug() << "Placeholder for query implementation.";
}