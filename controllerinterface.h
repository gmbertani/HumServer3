#pragma once

#include <QObject>
#include <QSerialPort>
#include <QHostAddress>
#include "settings.h"
#include "humatric_protocol.h"

class ControllerInterface : public QObject
{
    Q_OBJECT

public:
    explicit ControllerInterface(Settings &settingsRef, QObject *parent = nullptr);
    ~ControllerInterface();

    QString getSerialNumber();

private:
    QByteArray readBytes(int minBytes, int maxBytes);
    bool writeBytes(const QByteArray &data);
    QVariant handleResponse(const QByteArray &data);
    uint16_t computeCRC(const QByteArray &data) const;
    void parseSerialParams(const QString &paramString);

private:
    Settings &settings;
    QSerialPort *serial;
};
