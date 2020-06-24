#ifndef LSPMESSAGEBUILDER_H
#define LSPMESSAGEBUILDER_H

#include <QObject>
#include <QJsonDocument>

enum class LspMsgBuilderState {
    Headers, // building a header
    Payload, // reading JSON body
    Recovery, // error encountered, looking for next header
};

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
    QList<LspHeader> headers;

    QJsonDocument message;

    LspMessage(QList<LspHeader> headers, QJsonDocument message) : message(message) {
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
    LspMessageBuilder(QObject *parent = nullptr) : QObject(parent) {}

    void append(QByteArray *data);

signals:
    void emitLspMessage(LspMessage *msg);

private:
    LspMsgBuilderState state = LspMsgBuilderState::Headers;

    size_t pendingPayload = 0;

    QList<LspHeader> headers {};

    QByteArray buffer {};
};

#endif // LSPMESSAGEBUILDER_H
