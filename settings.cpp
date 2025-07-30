#include "settings.h"
#include <QDebug>
#include <QFileInfo>

Settings::Settings()
    : QSettings(QDir::homePath() + "/.humserver/config.ini", QSettings::IniFormat)
{
    qDebug() << "Config path:" << fileName();
    reset();
    //save();  //solo per primo file ini
}

void Settings::load()
{
    // Comm Params
    beginGroup("CommParams");
    serialPort = value("SerialPort").toString();
    serialParams = value("SerialParams").toString();
    controllerIP = value("ControllerIP").toString();
    controllerPort = value("ControllerPort").toInt();
    licenseServerUrl = value("LicenseServerUrl").toString();
    endGroup();

    // GUI
    beginGroup("GUI");
    language = value("Language").toString();
    unitSystem = value("UnitSystem").toString();
    dbUrl = value("MariaDBUrl").toString();
    dbUser = value("MariaDBUser").toString();
    dbPassword = value("MariaDBPassword").toString();
    endGroup();

    // Controller
    beginGroup("Controller");
    wifiSSID = value("SSID").toString();
    wifiPassword = value("Password").toString();
    sampleRate = value("SampleRate").toInt();
    channelMask = value("ChannelMask").toUInt();
    endGroup();

    // Database
    beginGroup("Database");
    dbType = value("Type").toString();
    dbAccountUser = value("User").toString();
    dbAccountPassword = value("Password").toString();
    dbAccountUrl = value("Url").toString();
    endGroup();

    qDebug() << "[Settings] Configuration loaded";
}

void Settings::save()
{
    // Comm Params
    beginGroup("CommParams");
    setValue("SerialPort", serialPort);
    setValue("SerialParams", serialParams);
    setValue("ControllerIP", controllerIP);
    setValue("ControllerPort", controllerPort);
    setValue("LicenseServerUrl", licenseServerUrl);
    endGroup();

    // GUI
    beginGroup("GUI");
    setValue("Language", language);
    setValue("UnitSystem", unitSystem);
    setValue("MariaDBUrl", dbUrl);
    setValue("MariaDBUser", dbUser);
    setValue("MariaDBPassword", dbPassword);
    endGroup();

    // Controller
    beginGroup("Controller");
    setValue("SSID", wifiSSID);
    setValue("Password", wifiPassword);
    setValue("SampleRate", sampleRate);
    setValue("ChannelMask", channelMask);
    endGroup();

    // Database
    beginGroup("Database");
    setValue("Type", dbType);
    setValue("User", dbAccountUser);
    setValue("Password", dbAccountPassword);
    setValue("Url", dbAccountUrl);
    endGroup();

    sync();
    qDebug() << "[Settings] Configurazione salvata";
}

void Settings::reset()
{
    //reset to factory settings

    // Comm Params
    serialPort = "COM1:";
    serialParams = "115200,8,n,1";
    controllerIP = "0.0.0.0";            //TODO: letto con GET_STATUS
    controllerPort = 2025;               //UDP port
    licenseServerUrl = "89.36.210.132";  //VPS Sirius


    // GUI
    language = "English";
    unitSystem = "Metric";
    dbUrl = "localhost";
    dbUser = "humserver";
    dbPassword = "p@Ran2aXtutti";

    // Controller
    wifiSSID = "ILMN";
    wifiPassword = "brtgpp65t08f205b";
    sampleRate = 100;
    channelMask = 0x3f;

    // Database
    dbType = "MariaDB";
    dbAccountUser = "humserver";
    dbAccountPassword = "p@Ran2aXtutti";
    dbAccountUrl = "localhost:3306";

    qDebug() << "[Settings] Configuration reset";
}
