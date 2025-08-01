#include "ControllerInterface.h"
#include <QDebug>
#include <QThread>

ControllerInterface::ControllerInterface(Settings &settingsRef, QObject *parent)
    : QObject(parent),
    settings(settingsRef),
    serial(new QSerialPort(this)),
    readerThread(nullptr),
    running(true)
{
    serial->setPortName(settings.serialPort);
    parseSerialParams(settings.serialParams);

    if (!serial->open(QIODevice::ReadWrite))
    {
        qCritical() << "Failed to open serial port:" << serial->errorString();
        running = false;
        return;
    }

    qDebug() << "Serial port opened:" << serial->portName();

    startReaderThread();
}

ControllerInterface::~ControllerInterface()
{
    stopReaderThread();

    if (serial->isOpen())
    {
        serial->close();
        qDebug() << "Serial port closed.";
    }
}

void ControllerInterface::parseSerialParams(const QString &paramString)
{
    QStringList parts = paramString.split(',', Qt::SkipEmptyParts);

    if (parts.size() == 4)
    {
        bool ok = false;

        int baudRate = parts[0].toInt(&ok);
        if (ok)
        {
            serial->setBaudRate(baudRate);
        }

        int dataBits = parts[1].toInt(&ok);
        if (ok)
        {
            if (dataBits == 7)
            {
                serial->setDataBits(QSerialPort::Data7);
            }
            else
            {
                serial->setDataBits(QSerialPort::Data8);
            }
        }

        QString parity = parts[2].trimmed().toLower();
        if (parity == "e")
        {
            serial->setParity(QSerialPort::EvenParity);
        }
        else if (parity == "o")
        {
            serial->setParity(QSerialPort::OddParity);
        }
        else
        {
            serial->setParity(QSerialPort::NoParity);
        }

        int stopBits = parts[3].toInt(&ok);
        if (ok)
        {
            if (stopBits == 2)
            {
                serial->setStopBits(QSerialPort::TwoStop);
            }
            else
            {
                serial->setStopBits(QSerialPort::OneStop);
            }
        }
    }
    else
    {
        qWarning() << "Invalid serialParams format, expected: baud,dataBits,parity,stopBits";
    }
}

void ControllerInterface::startReaderThread()
{
    readerThread = QThread::create([this]()
    {
       while (running)
       {
           if (!serial->waitForReadyRead(50))
           {
               continue;
           }

           QByteArray data = serial->readAll();

           if (!data.isEmpty())
           {
               QMutexLocker locker(&bufferMutex);

               for (char byte : data)
               {
                   if (circularBuffer.size() < bufferMaxSize)
                   {
                       circularBuffer.enqueue(byte);
                   }
                   else
                   {
                       // Optional: log overflow
                       break;
                   }
               }
           }
       }
    });

    readerThread->start();
}

void ControllerInterface::stopReaderThread()
{
    if (readerThread)
    {
        running = false;
        readerThread->quit();
        readerThread->wait();

        delete readerThread;
        readerThread = nullptr;
    }
}

QByteArray ControllerInterface::readBytes(int minBytes, int maxBytes)
{
    QByteArray data;

    while (true)
    {
        {
            QMutexLocker locker(&bufferMutex);

            if (circularBuffer.size() >= minBytes)
            {
                int toRead = qMin(maxBytes, circularBuffer.size());

                for (int i = 0; i < toRead; ++i)
                {
                    data.append(circularBuffer.dequeue());
                }

                break;
            }
        }

        if (!running)
        {
            break;
        }

        QThread::msleep(1);
    }

    return data;
}


bool ControllerInterface::writeBytes(const QByteArray &data)
{
    if (serial == nullptr || !serial->isOpen())
    {
        return false;
    }

    qint64 written = serial->write(data);

    return written == data.size();
}

int ControllerInterface::bytesAvailable() const
{
    QMutexLocker locker(&bufferMutex);
    return circularBuffer.size();
}


QString ControllerInterface::getSerialNumber()
{
    QByteArray command("\xFE\xED\x01\x00\x24\x4a\x9f\x04\x04");
    if (writeBytes(command))
    {
        QByteArray response(readBytes(6,64));
        QVariant serialID = handleResponse(response);
        return serialID.toString();
    }
    return {};
}

uint16_t ControllerInterface::computeCRC(const QByteArray &data) const
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data.constData());
    size_t length = static_cast<size_t>(data.size());

    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; ++i)
    {
        crc ^= static_cast<uint16_t>(bytes[i]) << 8;

        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

QVariant ControllerInterface::handleResponse(const QByteArray &data)
{
    if (data.size() < 6)
    {
        qWarning() << "Response too short";
        return {};
    }

    const int dataLen = data.size();
    const uint8_t *raw = reinterpret_cast<const uint8_t *>(data.constData());

    uint16_t header = static_cast<uint16_t>(raw[1] << 8 | raw[0]);

    if (header != RSP_HEADER_MARKER)
    {
        qWarning() << "Invalid header marker";
        return {};
    }

    // CRC: last 2 bytes
    uint16_t receivedCRC = static_cast<uint16_t>(raw[dataLen - 2] | (raw[dataLen - 1] << 8));
    uint16_t expectedCRC = computeCRC(data.left(dataLen - 2));


    if (receivedCRC != expectedCRC)
    {
        qWarning() << "CRC mismatch: expected" << expectedCRC << "received" << receivedCRC;
        return {};
    }

    uint8_t command = raw[2];

    switch (command)
    {
        case CMD_GET_SERIAL_NUMBER:
        {
            if (data.size() < sizeof(T_SerialNumberResponse))
            {
                return {};
            }

            const T_SerialNumberResponse *rsp = reinterpret_cast<const T_SerialNumberResponse *>(raw);
            return QString::fromLatin1(rsp->serialID);
        }

        case CMD_GET_FW_VERSION:
        {
            if (data.size() < sizeof(T_FirmwareVersionResponse))
            {
                return {};
            }

            const T_FirmwareVersionResponse *rsp = reinterpret_cast<const T_FirmwareVersionResponse *>(raw);
            QVariantMap map;
            map["stm32FW"] = QString::fromLatin1(rsp->stm32FW);
            map["esp32FW"] = QString::fromLatin1(rsp->esp32FW);
            return map;
        }

        case CMD_GET_SAMPLING_RATE:
        {
            if (data.size() < sizeof(T_SampleRateResponse))
            {
                return {};
            }

            const T_SampleRateResponse *rsp = reinterpret_cast<const T_SampleRateResponse *>(raw);
            return static_cast<int>(rsp->sampleRate);
        }

        case CMD_GET_CHANNEL_MASK:
        {
            if (data.size() < sizeof(T_ChannelMaskResponse))
            {
                return {};
            }

            const T_ChannelMaskResponse *rsp = reinterpret_cast<const T_ChannelMaskResponse *>(raw);
            return static_cast<int>(rsp->channelMask);
        }

        case CMD_GET_STATUS:
        {
            if (data.size() < sizeof(T_StatusResponse))
            {
                return {};
            }

            const T_StatusResponse *rsp = reinterpret_cast<const T_StatusResponse *>(raw);
            QVariantMap map;
            map["pad"] = rsp->padAddress;
            map["temperature"] = rsp->temperature;
            map["errorFlags"] = static_cast<quint32>(rsp->errorFlags);
            map["serialParams"] = QString::fromLatin1(rsp->serialParams);
            map["wifiSSID"] = QString::fromLatin1(rsp->wifiSSID);
            map["wifiIP"] = QHostAddress(rsp->wifiIP[0] << 24 |
                                         rsp->wifiIP[1] << 16 |
                                         rsp->wifiIP[2] << 8 |
                                         rsp->wifiIP[3]).toString();
            return map;
        }

        case CMD_GET_FRAME:
        {
            if (data.size() < sizeof(T_Frame))
            {
                return {};
            }

            const T_Frame *frame = reinterpret_cast<const T_Frame *>(raw);
            return QVariant::fromValue(*frame);
        }

        default:
        {
            if (data.size() >= sizeof(T_AckResponse))
            {
                const T_AckResponse *ack = reinterpret_cast<const T_AckResponse *>(raw);
                if (ack->ackCode == 0x06)
                {
                    return static_cast<int>(ack->ackCode);
                }
            }

            if (data.size() >= sizeof(T_NackResponse))
            {
                const T_NackResponse *nak = reinterpret_cast<const T_NackResponse *>(raw);
                if (nak->nakCode == 0x15)
                {
                    return static_cast<int>(nak->errorCode);
                }
            }

            return {};
        }
    }
}
