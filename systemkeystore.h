#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QDate>
#include "humtoken.h"


class SystemKeyStore : public QObject
{
    Q_OBJECT

public:
    explicit SystemKeyStore(QObject *parent = nullptr);

    QByteArray getToken();
    void createTempToken(QString& ctrlID);
    void setToken(const QByteArray &token);
    QByteArray getTempToken() const
    {
        return tempToken.toByteArray();
    }

    QString renewalDate()
    {
        return tempToken.getCheckTime().toString();
    }

    bool isTokenStillValid(QString ctrlID);
    bool isTokenExpired(QString& ctrlID);

private:
    QSettings *registry;
    HumToken tempToken;

    QByteArray readToken();
    void writeToken(const QByteArray &token);
    QByteArray getFingerprint();  // UUID derived from hardware identity


};
