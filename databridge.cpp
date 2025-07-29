#include "databridge.h"
#include <QDebug>
#include <QRandomGenerator>

DataBridge::DataBridge(QObject *parent) : QObject(parent) {
    m_dataList << "Iniziale 1" << "Iniziale 2";
}

QStringList DataBridge::dataList() const {
    return m_dataList;
}

void DataBridge::triggerData() {
    m_dataList.clear();
    for (int i = 0; i < 5; ++i)
        m_dataList << QString("Valore %1").arg(QRandomGenerator::global()->bounded(0, 65536));
    emit dataListChanged();
}

void DataBridge::sendLog(const QString &msg) {
    qDebug() << "[Browser log]: " << msg;
    emit logSent(QString("Qt ha ricevuto: %1").arg(msg));
}
