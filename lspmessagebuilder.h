#ifndef LSPMESSAGEBUILDER_H
#define LSPMESSAGEBUILDER_H

#include <QObject>
#include <QDateTime>
#include <QJsonDocument>

enum class LspMsgBuilderState {
    Headers, // building a header
    Payload, // reading JSON body
    Recovery, // error encountered, looking for next header
};

enum class LspEntity {
    Client,
    Server,
};

QString lspEntityToQString(LspEntity entity);

struct LspHeader {
    QString raw;
    QString key;
    QString value;

    LspHeader(QString raw, QString key, QString value) : raw(raw), key(key.toUpper()), value(value) {}

    QString getKey() {
        return key;
    }

    QString getValue() {
        return value;
    }

    bool keyEquals(QString other) {
        return getKey().compare(other, Qt::CaseInsensitive) == 0;
    }
};


struct LspMessage {
    qint64 timestamp;

    LspEntity sender;

    QList<LspHeader> headers;

    QJsonDocument message;

    LspMessage(LspEntity sender, QList<LspHeader> headers, QJsonDocument message) : LspMessage(QDateTime::currentMSecsSinceEpoch(), sender, headers, message) {}

    LspMessage(qint64 timestamp, LspEntity sender, QList<LspHeader> headers, QJsonDocument message) : message(message) {
        this->timestamp = timestamp;
        this->sender = sender;
        this->headers = QList<LspHeader> {};
        for (LspHeader header : headers) {
            this->headers.append(header);
        }
    }
};


/**
 * Takes a stream of characters, emits well formed LSP messages or errors
 */
class LspMessageBuilder : public QObject
{
    Q_OBJECT

public:
    LspMessageBuilder(LspEntity source, QObject *parent = nullptr) : QObject(parent), source(source) {}

    void append(QByteArray *data);

signals:
    void emitLspMessage(LspMessage *msg);

private:
    LspEntity source;

    LspMsgBuilderState state = LspMsgBuilderState::Headers;

    size_t pendingPayload = 0;

    QList<LspHeader> headers {};

    QByteArray buffer {};
};

#endif // LSPMESSAGEBUILDER_H
