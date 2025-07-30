#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

class SystemKeyStore : public QObject
{
    Q_OBJECT

public:
    explicit SystemKeyStore(QObject *parent = nullptr);

    QString getToken();
    void setToken(const QString &token);

    QString getFingerprint();  // UUID derived from hardware identity

private:
    QSettings *settings;

    QString readToken();
    void writeToken(const QString &token);
};
