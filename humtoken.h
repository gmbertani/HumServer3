#pragma once

#include <QByteArray>
#include <QDate>
#include <QDataStream>
#include <QtGlobal>
#include <QIODevice>
#include <QtEndian>
#include <cstring>   // per memcpy, memset

class HumToken
{
public:
    HumToken()
    {
        controllerID.clear();
        validatedKey.clear();
        fingerprint.clear();
        checkTime = QDate::currentDate();
    }

    void setControllerID(const char *id)
    {
        if (!id)
        {
            qWarning() << Q_FUNC_INFO << "Null pointer passed as controller ID";
            controllerID.clear();
            return;
        }

        QString s = QString::fromUtf8(id).trimmed();

        if (s.length() > 20)
        {
            qWarning() << Q_FUNC_INFO << "Controller ID too long, trimming to 20 characters";
            s = s.left(20);
        }

        controllerID = s;
    }

    void setControllerID(const QString &id)
    {
        controllerID = id.trimmed().left(20);
    }

    void setFingerprint(const QByteArray &fp)
    {
        fingerprint = fp;
    }

    void setValidatedKey(const QByteArray &ak)
    {
        validatedKey = ak;
    }

    void setCheckTime(const QDate &date)
    {
        checkTime = date;
    }

    /*
     * non serve
    const char *getControllerID() const
    {
        return controllerID.toLatin1().constData();
    }
    */

    const QString &getControllerID() const
    {
        return controllerID;
    }

    const QByteArray &getFingerprint() const
    {
        return fingerprint;
    }

    const QByteArray &getValidatedKey() const
    {
        return validatedKey;
    }

    const QDate &getCheckTime() const
    {
        return checkTime;
    }

    QByteArray toByteArray() const
    {
        QByteArray out;

        out.append(controllerID.toUtf8().leftJustified(20, '\0', false));
        out.append(fingerprint.leftJustified(32, '\0', false));

        quint32 timestamp = checkTime.startOfDay().toSecsSinceEpoch();
        quint32 be = qToBigEndian(timestamp);
        out.append(reinterpret_cast<const char*>(&be), 4);

        out.append(validatedKey.leftJustified(32, '\0', false));

        return out;
    }


    static HumToken fromByteArray(const QByteArray &data)
    {
        HumToken token;

        if (data.size() < 88)  // Check lunghezza minima
        {
            qWarning() << "HumToken::fromByteArray() - invalid token size:" << data.size();
            return token; // probabilmente oggetto vuoto
        }

        // controllerID: 20 byte
        token.controllerID = QString::fromUtf8(data.constData(), 20).trimmed();

        // fingerprint: 32 byte
        token.fingerprint = QByteArray(data.constData() + 20, 32);

        // checkTime: 4 byte, uint32 big endian → QDate
        quint32 timestamp;
        memcpy(&timestamp, data.constData() + 52, 4);
        timestamp = qFromBigEndian(timestamp);
        token.checkTime = QDateTime::fromSecsSinceEpoch(timestamp).date();

        // validatedKey: 32 byte
        token.validatedKey = QByteArray(data.constData() + 56, 32);

        return token;
    }

private:


private:
    QString  controllerID;     // Fixed-size 20 chr null-terminated ASCII ID
    QByteArray fingerprint;    // Hardware fingerprint (32 bytes typical)
    QDate checkTime;           // Reference date (e.g., license check)
    QByteArray validatedKey;  // Activation key
};
