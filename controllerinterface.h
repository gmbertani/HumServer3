#pragma once

#include <QObject>
#include <QSerialPort>
#include <QThread>
#include <QMutex>
#include <QQueue>
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

    uint8_t rxBuffer[64];

    QByteArray readBytes(int minBytes, int maxBytes);
    bool writeBytes(const QByteArray &data);
    int bytesAvailable() const;
    QVariant handleResponse(const QByteArray &data);
    uint16_t computeCRC(const QByteArray &data) const;


    void startReaderThread();
    void stopReaderThread();
    void parseSerialParams(const QString &paramString);

private:
    Settings &settings;
    QSerialPort *serial;
    QThread *readerThread;

    mutable QMutex bufferMutex;
    QQueue<char> circularBuffer;
    static const int bufferMaxSize = 1024;
    volatile bool running;
};
