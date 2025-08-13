#pragma once

#include <QSettings>
#include <QDir>
#include <QString>
#include <QStandardPaths>



enum LogLevels_t
{
    HUM_LOG_NONE,               // do not create a program log
    HUM_LOG_FATAL,              // only fatal messages (crashes) are logged
    HUM_LOG_CRITICAL,           // logs fatal and critical messages (alerts about possible crashes)
    HUM_LOG_WARNINGS,           // logs warnings too (the program does not behave as expected)
    HUM_LOG_INFO,               // logs info messages too

    HUM_LOG_ALL                 // logs everything including debug messages (huge logs)
};

extern LogLevels_t logLevel;    //from main.cpp

#define MYDEBUG             if(logLevel >= HUM_LOG_ALL) qDebug() << __FILE__ << ":" << __LINE__
#define MYINFO              if(logLevel >= HUM_LOG_INFO) qInfo() << __FILE__ << ":" << __LINE__
#define MYWARNING           if(logLevel >= HUM_LOG_WARNINGS) qWarning() << __FILE__ << ":" << __LINE__
#define MYCRITICAL          if(logLevel >= HUM_LOG_CRITICAL) qCritical() << __FILE__ << ":" << __LINE__
#define MYFATAL(str,arg)    if(logLevel >= HUM_LOG_FATAL) qFatal(str,arg)


class Settings : public QSettings
{
public:
    Settings();

    void load();
    void save();
    void reset();

    // Sezioni:

    // License
    QString licenseServerSite;
    QString serverCertName;             //il certificato si trova sempre insieme all'exe
    QByteArray activationKeyBytes;      //questa non ha requisiti di sicurezza, diversamente dalla validated key
    int licenseServerPort;

    // GUI
    QString language;
    QString unitSystem;
    QString dbUrl;
    QString dbUser;
    QString dbPassword;
    QString indexPath;

    // Controller
    QString serialPort;
    QString serialParams;
    QString controllerIP;
    QString wifiSSID;
    QString wifiPassword;
    int controllerPort = 0;
    int sampleRate = 0;
    quint32 channelMask = 0;

    // Database
    QString dbType;
    QString dbAccountUser;
    QString dbAccountPassword;
    QString dbAccountUrl;
};
