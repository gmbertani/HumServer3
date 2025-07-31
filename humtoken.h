#pragma once

#include <QByteArray>
#include <QDate>
#include <QDataStream>
#include <QtGlobal>
#include <QIODevice>
#include <cstring>   // per memcpy, memset

class HumToken
{
public:
    HumToken()
    {
        clearControllerID();
    }

    void setControllerID(const char *id)
    {
        Q_ASSERT_X(strlen(id) > (sizeof(controllerID)-1), __func__, "Invalid controller serial number");
        clearControllerID();
        if (id)
        {
            qstrncpy(controllerID, id, sizeof(controllerID));
        }
    }

    void setFingerprint(const QByteArray &fp)
    {
        fingerprint = fp;
    }

    void setCheckTime(const QDate &date)
    {
        checkTime = date;
    }

    const char *getControllerID() const
    {
        return controllerID;
    }

    const QByteArray &getFingerprint() const
    {
        return fingerprint;
    }

    const QDate &getCheckTime() const
    {
        return checkTime;
    }

    QByteArray toByteArray() const
    {
        QByteArray out;
        QDataStream stream(&out, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_0);

        stream.writeRawData(controllerID, sizeof(controllerID));
        stream << fingerprint << checkTime;

        return out;
    }

    static HumToken fromByteArray(const QByteArray &data)
    {
        HumToken token;
        QDataStream stream(data);
        stream.setVersion(QDataStream::Qt_6_0);

        stream.readRawData(token.controllerID, sizeof(token.controllerID));
        stream >> token.fingerprint >> token.checkTime;

        return token;
    }

private:
    void clearControllerID()
    {
        memset(controllerID, 0, sizeof(controllerID));
    }

private:
    char controllerID[20];     // Fixed-size null-terminated ASCII ID
    QByteArray fingerprint;    // Hardware fingerprint (32 bytes typical)
    QDate checkTime;           // Reference date (e.g., license check)
};
