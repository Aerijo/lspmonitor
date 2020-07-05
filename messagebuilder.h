#ifndef MESSAGEBUILDER_H
#define MESSAGEBUILDER_H

#include <memory>
#include <utility>

#include <QObject>
#include <QDateTime>

#include "framebuilder.h"
#include "option.h"

namespace MessageBuilder {

class HeaderParser {
public:
    HeaderParser(QString value);

    enum class Kind {
        Token,
        OptionalWhitespace,
        MandatoryWhitespace,
        ParamValue,
        QuotedString,
    };

    QString parse(char c);

    QString parse(Kind kind);

    bool finished();

    bool errorOccurred();

private:
    void setError(int location);

    QString value;

    int index = 0;

    int errorIndex = -1;
};

struct ContentTypeHeader {
    QString type;

    QString subtype;

    QVector<std::pair<QString, QString>> parameters;

    ContentTypeHeader(QString type, QString subtype) : ContentTypeHeader(type, subtype, QVector<std::pair<QString, QString>>()) {}

    ContentTypeHeader(QString type, QString subtype, QVector<std::pair<QString, QString>> params) : type(type), subtype(subtype), parameters(params) {}

    static option<ContentTypeHeader> fromHeaderValue(QString value);
};

/**
 * Represents a well formed UTF-8 encoded JSON message, derived from a
 * payload from a Frame
 */
struct Message {
    /** The time that the message frame was fully received by */
    qint64 timestamp;

    /** The contents of the message */
    QJsonDocument contents;

    /** The size in bytes of the Frame */
    int size;

    Message(FrameBuilder::Frame frame, QJsonDocument contents);
    Message(qint64 timestamp, QJsonDocument contents, int size);
};

/**
 * Processes Frame's into JSON messages. Handles things like encodings
 * and JSON parse errors. Validating the result against the LSP schema
 * is done in a later stage.
 */
class MessageBuilder : public QObject {
    Q_OBJECT
public:
    explicit MessageBuilder(QObject *parent = nullptr);

signals:
    void emitMessage(Message message);

    void emitError();

public slots:
    void onFrame(FrameBuilder::Frame frame);

};

}



#endif // MESSAGEBUILDER_H
