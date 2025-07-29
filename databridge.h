#ifndef DATABRIDGE_H
#define DATABRIDGE_H

#include <QObject>
#include <QStringList>

class DataBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList dataList READ dataList NOTIFY dataListChanged)

public:
    explicit DataBridge(QObject *parent = nullptr);

    Q_INVOKABLE void triggerData();
    Q_INVOKABLE void sendLog(const QString &msg);

    QStringList dataList() const;

signals:
    void dataListChanged();
    void logSent(const QString &msg);

private:
    QStringList m_dataList;
};

#endif // DATABRIDGE_H
