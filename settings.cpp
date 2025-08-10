#include "settings.h"
#include <QDebug>
#include <QFileInfo>

Settings::Settings()
    : QSettings(QDir::homePath() + "/.humserver/config.ini", QSettings::IniFormat)
{
    MYDEBUG << "Config path:" << fileName();
    reset();
    //save();  //solo per primo file ini
}

void Settings::load()
{
    if (QFile::exists(fileName()))
    {
        // Comm Params
        beginGroup("CommParams");

        licenseServerSite = value("LicenseServerSite").toString();
        activationKey = value("ActivationKey").toString();
        licenseServerPort = value("LicenseServerPort").toInt();
        endGroup();

        // GUI
        beginGroup("GUI");
        language = value("Language").toString();
        unitSystem = value("UnitSystem").toString();
        dbUrl = value("MariaDBUrl").toString();
        dbUser = value("MariaDBUser").toString();
        dbPassword = value("MariaDBPassword").toString();
        indexPath = value("IndexPath").toString();
        endGroup();

        // Controller
        beginGroup("Controller");
        serialPort = value("SerialPort").toString();
        serialParams = value("SerialParams").toString();
        controllerIP = value("ControllerIP").toString();
        wifiSSID = value("SSID").toString();
        wifiPassword = value("Password").toString();
        controllerPort = value("ControllerPort").toInt();
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

        MYDEBUG << "[Settings] Configuration loaded";
    }
     MYWARNING << fileName() << " not found, taking factory settings";
}

void Settings::save()
{
    // Comm Params
    beginGroup("License");
    setValue("LicenseServerSite", licenseServerSite);
    setValue("ActivationKey", activationKey);
    setValue("LicenseServerPort", licenseServerPort);
    endGroup();

    // GUI
    beginGroup("GUI");
    setValue("Language", language);
    setValue("UnitSystem", unitSystem);
    setValue("MariaDBUrl", dbUrl);
    setValue("MariaDBUser", dbUser);
    setValue("MariaDBPassword", dbPassword);
    setValue("IndexPath", indexPath);
    endGroup();

    // Controller
    beginGroup("Controller");
    setValue("SerialPort", serialPort);
    setValue("SerialParams", serialParams);
    setValue("ControllerIP", controllerIP);
    setValue("SSID", wifiSSID);
    setValue("Password", wifiPassword);
    setValue("ControllerPort", controllerPort);
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
    MYDEBUG << "[Settings] Configurazione salvata";
}

void Settings::reset()
{
    //reset to factory settings

    // License

    licenseServerSite = "lic.siriuslogic.it"; //89.36.210.132 VPS Sirius (siriuslogic.it)
    activationKey = "";
    licenseServerPort = 5678;

    // GUI
    language = "English";
    unitSystem = "Metric";
    dbUrl = "localhost";
    dbUser = "humserver";
    dbPassword = "p@Ran2aXtutti";
    indexPath = QDir::toNativeSeparators("c:/Users/zot/Documents/Hum/Software/HumGUI/index.html");

    // Controller
    serialPort = "COM1";
    serialParams = "115200,8,n,1";
    controllerIP = "0.0.0.0";            //TODO: letto con GET_STATUS
    controllerPort = 2025;               //UDP port
    wifiSSID = "ILMN";
    wifiPassword = "brtgpp65t08f205b";
    sampleRate = 100;
    channelMask = 0x3f;

    // Database
    dbType = "MariaDB";
    dbAccountUser = "humserver";
    dbAccountPassword = "p@Ran2aXtutti";
    dbAccountUrl = "localhost:3306";

    MYDEBUG << "[Settings] Configuration reset";
}
