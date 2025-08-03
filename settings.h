#pragma once

#include <QSettings>
#include <QDir>
#include <QString>
#include <QStandardPaths>

class Settings : public QSettings {
public:
    Settings();

    void load();
    void save();
    void reset();

    // Sezioni: CommParams
    QString serialPort;
    QString serialParams;
    QString controllerIP;
    int controllerPort = 0;
    QString licenseServerUrl;
    QString activationKey;     //questa non ha requisiti di sicurezza, diversamente dalla validated key

    // GUI
    QString language;
    QString unitSystem;
    QString dbUrl;
    QString dbUser;
    QString dbPassword;

    // Controller
    QString wifiSSID;
    QString wifiPassword;
    int sampleRate = 0;
    quint32 channelMask = 0;

    // Database
    QString dbType;
    QString dbAccountUser;
    QString dbAccountPassword;
    QString dbAccountUrl;
};
