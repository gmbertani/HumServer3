#pragma once

#include <QWebChannelAbstractTransport>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

class WebSocketTransport : public QWebChannelAbstractTransport {
    Q_OBJECT
public:
    explicit WebSocketTransport(QWebSocket *socket, QObject *parent = nullptr)
        : QWebChannelAbstractTransport(parent), m_socket(socket) {
        connect(m_socket, &QWebSocket::textMessageReceived,
                this, &WebSocketTransport::onTextMessageReceived);
    }

    void sendMessage(const QJsonObject &message) override {
        QJsonDocument doc(message);
        m_socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    }

private slots:
    void onTextMessageReceived(const QString &message) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && doc.isObject())
            emit messageReceived(doc.object(), this);
        else
            qWarning() << "Errore parsing JSON:" << error.errorString();
    }

private:
    QWebSocket *m_socket;
};
