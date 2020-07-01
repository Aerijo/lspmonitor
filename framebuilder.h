#ifndef FRAMEBUILDER_H
#define FRAMEBUILDER_H

#include <QtCore>

namespace FrameBuilder {

/**
 * A message header, parsed as per https://tools.ietf.org/html/rfc7230#section-3.2
 */
struct Header {
    /** The name of the field */
    QString name;

    /** The value of the field */
    QString value;

    Header(QString name, QString value);
};

/**
 * Represents a whole message extracted from the stream
 */
struct Frame {
    /** The time this frame was fully received */
    qint64 timestamp;

    /** The index into the stream this frame starts on (index of first header character) */
    size_t frameStart;

    /** The index into the stream this frame ends on (exclusive, next frames frameStart if no errors) */
    size_t frameEnd;

    /** The index into the stream this frames payload starts on */
    size_t payloadStart;

    /** The headers of the message, in order. May contain duplicates. */
    QVector<Header> headers;

    /** The full payload of the message, as specified by the Content-Length header */
    QByteArray payload;

    /** If the message was built in recovery mode */
    bool fromRecoveryMode;

    Frame(qint64 timestamp, size_t frameStart, size_t frameEnd, size_t payloadStart, QVector<Header> headers, QByteArray payload, bool recovery=false);
};

struct StreamError {
    /** Where the error was discovered (offset from the beginning of the stream) */
    size_t globalOffset;

    /** Where the error was discovered (offset from the start of the current message) */
    size_t localOffset;

    /** The type of error detected */
    enum class Kind {
        /** The stream defines mutiple Content-Length headers, possibly conflicting each other */
        MultipleContentLength,

        /** There is no Content-Length header */
        MissingContentLength,

        /** The Content-Length header value is not an integer */
        ContentLengthNaN,

        /** The Content-Length header value is negative */
        ContentLengthNegative,

        /** The header name is empty */
        MissingHeaderName,

        /** The input character is unexpected */
        UnexpectedCharacter,
    } kind;

    StreamError(size_t globalOffset, size_t localOffset, Kind kind);

    static QString kindToQString(Kind kind);

    QString toQString();
};

/**
 * Takes in a stream of bytes and emits message frames, or errors to indicate
 * issues with the framing. Note the frame is just headers and an array of bytes;
 * parsing into a well formed LSP message happens elsewhere.
 */
class FrameBuilder : public QObject {
    Q_OBJECT

public:
    FrameBuilder(QObject* parent = nullptr);

    void operator<<(const QByteArray &data);
    void operator<<(char d);

signals:
    /** Fired for each well formed message in the stream */
    void emitFrame(Frame frame);

    /** Fired whenever an error occurs. The builder will try to find the next valid message */
    void emitError(StreamError error);

public slots:
    void onInput(QByteArray input);

private:
    enum class State {
        Headers,
        Payload,
    } state = State::Headers;

    enum class HeadersState {
        NameStart,
        Name,
        Value,
        ValueEnd,
        End,
    } headersState = HeadersState::NameStart;

    int recoveryState = 0;

    long pending = 0;

    size_t offset = 0;

    size_t frameStart = 0;

    QByteArray buffer;

    QVector<Header> headers;

    QString headerName;

    QString headerValue;

    void handleError(StreamError::Kind kind);

    void initialisePayload();

    void emitPayload();

    long getPayloadSizeFromHeaders();

    void appendHeader(char c);

    void appendPayload(char c);
};

}

#endif // FRAMEBUILDER_H
