#include "ControllerInterface.h"
#include <QDebug>

ControllerInterface::ControllerInterface(Settings &settingsRef, QObject *parent)
    : QObject(parent),
      settings(settingsRef),
      serial(new QSerialPort(this))
{
    serial->setPortName(settings.serialPort);
    parseSerialParams(settings.serialParams);

    if (!serial->open(QIODevice::ReadWrite))
    {
        MYCRITICAL << "Failed to open serial port: " << settings.serialPort << " : " << serial->errorString();
        return;
    }

    MYDEBUG << "Serial port opened:" << serial->portName();
}

ControllerInterface::~ControllerInterface()
{
    if (serial->isOpen())
    {
        serial->close();
        MYDEBUG << "Serial port closed.";
    }
}

void ControllerInterface::parseSerialParams(const QString &paramString)
{
    QStringList parts = paramString.split(',', Qt::SkipEmptyParts);

    if (parts.size() == 4)
    {
        bool ok = false;

        int baudRate = parts[0].toInt(&ok);
        if (ok) serial->setBaudRate(baudRate);

        int dataBits = parts[1].toInt(&ok);
        if (ok)
        {
            serial->setDataBits(dataBits == 7 ? QSerialPort::Data7 : QSerialPort::Data8);
        }

        QString parity = parts[2].trimmed().toLower();
        if (parity == "e") serial->setParity(QSerialPort::EvenParity);
        else if (parity == "o") serial->setParity(QSerialPort::OddParity);
        else serial->setParity(QSerialPort::NoParity);

        int stopBits = parts[3].toInt(&ok);
        if (ok)
        {
            serial->setStopBits(stopBits == 2 ? QSerialPort::TwoStop : QSerialPort::OneStop);
        }
    }
    else
    {
        MYWARNING << "Invalid serialParams format, expected: baud,dataBits,parity,stopBits";
    }
}

QByteArray ControllerInterface::readBytes(int minBytes, int maxBytes)
{
    QByteArray data;

    while (serial->isOpen() && data.size() < minBytes)
    {
        if (!serial->waitForReadyRead(100))
        {
            continue; // aspetta ancora dati
        }

        QByteArray chunk = serial->read(maxBytes - data.size());
        if (!chunk.isEmpty())
        {
            data.append(chunk);
        }
    }

    return data;
}

bool ControllerInterface::writeBytes(const QByteArray &data)
{
    if (!serial || !serial->isOpen())
    {
        return false;
    }

    qint64 written = serial->write(data);
    if (!serial->waitForBytesWritten(1000))
    {
        return false;
    }

    return written == data.size();
}

QString ControllerInterface::getSerialNumber()
{
    MYDEBUG << __func__ << "()";

    QByteArray command("\xFE\xED\x01\x00\x24\x4a\x9f\x04\x04", 9);
    if (writeBytes(command))
    {
        MYDEBUG << __func__ << "() command sent, waiting for answer";
        QByteArray response = readBytes(6, 64);
        QVariant serialID = handleResponse(response);
        MYDEBUG << __func__ << "() got SID:" << serialID.toString();
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
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc;
}

QVariant ControllerInterface::handleResponse(const QByteArray &data)
{
    if (data.size() < 6)
    {
        MYWARNING << "Response too short";
        return {};
    }

    const int dataLen = data.size();
    const uint8_t *raw = reinterpret_cast<const uint8_t *>(data.constData());

    uint16_t header = static_cast<uint16_t>(raw[0] << 8 | raw[1]);
    if (header != RSP_HEADER_MARKER)
    {
        MYWARNING << "Invalid header marker";
        return {};
    }

    uint16_t receivedCRC = static_cast<uint16_t>(raw[dataLen - 4] | (raw[dataLen - 3] << 8));
    uint16_t expectedCRC = computeCRC(data.mid(2, dataLen - 6)); //esclude header, CRC e footer dal conto
    if (receivedCRC != expectedCRC)
    {
        MYWARNING << "CRC mismatch: expected" << expectedCRC << "received" << receivedCRC;
        return {};
    }

    uint8_t command = raw[3];

    switch (command)
    {
        case CMD_GET_SERIAL_NUMBER:
            if (data.size() < sizeof(T_SerialNumberResponse)) return {};
            return QString::fromLatin1(reinterpret_cast<const T_SerialNumberResponse *>(raw)->serialID);

        case CMD_GET_FW_VERSION:
            if (data.size() < sizeof(T_FirmwareVersionResponse)) return {};
            {
                const T_FirmwareVersionResponse *rsp = reinterpret_cast<const T_FirmwareVersionResponse *>(raw);
                QVariantMap map;
                map["stm32FW"] = QString::fromLatin1(rsp->stm32FW);
                map["esp32FW"] = QString::fromLatin1(rsp->esp32FW);
                return map;
            }

        default:
            return {};
    }
}
