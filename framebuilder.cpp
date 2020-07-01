#include "framebuilder.h"
#include "asciiparsing.h"

namespace FrameBuilder {

Header::Header(QString name, QString value) : name(name), value(value) {}

Frame::Frame(qint64 timestamp, size_t gStart, size_t gEnd, size_t pOff, QVector<Header> headers, QByteArray payload, bool recovery) : timestamp(timestamp), frameStart(gStart), frameEnd(gEnd), payloadStart(pOff), headers(headers), payload(payload), fromRecoveryMode(recovery) {}

StreamError::StreamError(size_t gOffset, size_t lOffset, Kind kind) : globalOffset(gOffset), localOffset(lOffset), kind(kind) {}

QString StreamError::kindToQString(Kind kind) {
    switch (kind) {
        case Kind::ContentLengthNaN:
            return "Content-Length header value is not a number";
        case Kind::MissingHeaderName:
            return "Header field missing name";
        case Kind::UnexpectedCharacter:
            return "Unexpected character in stream";
        case Kind::MissingContentLength:
            return "Missing Content-Length header";
        case Kind::ContentLengthNegative:
            return "Content-Length value is negative";
        case Kind::MultipleContentLength:
            return "Content-Length is defined multiple times";
    }
}

QString StreamError::toQString() {
    return kindToQString(kind);
}

FrameBuilder::FrameBuilder(QObject* parent) : QObject(parent) {}

void FrameBuilder::onInput(QByteArray input) {
    for (char c : input) {
        switch (state) {
            case State::Headers:
                appendHeader(c);
                break;
            case State::Payload:
                appendPayload(c);
                break;
        }

        offset += 1;
    }
}

void FrameBuilder::operator<<(const QByteArray &data) {
    onInput(data);
}

void FrameBuilder::operator<<(char c) {
    onInput(QByteArray().append(c));
}

void FrameBuilder::appendHeader(char c) {
    switch (headersState) {
        case HeadersState::NameStart:
            if (c == ':') {
                // Error: Must have 1 or more token characters
                handleError(StreamError::Kind::MissingHeaderName);
                return;
            } else if (c == '\r') {
                headersState = HeadersState::End;
                break;
            }
            headersState = HeadersState::Name;
        case HeadersState::Name:
            if (isTchar(c)) {
                headerName += c;
            } else if (c == ':') {
                headersState = HeadersState::Value;
            } else {
                // Error: expected token character or ':' delimiter
                handleError(StreamError::Kind::UnexpectedCharacter);
                return;
            }
            break;
        case HeadersState::Value:
            if (isHorizontalWhitespace(c) || isVchar(c)) {
                headerValue += c;
            } else if (c == '\r') {
                // End of header
                headersState = HeadersState::ValueEnd;
            } else {
                // Error: expected whitespace or visible ASCII character
                handleError(StreamError::Kind::UnexpectedCharacter);
                return;
            }
            break;
        case HeadersState::ValueEnd:
            if (c == '\n') {
                headers.append(Header(headerName, headerValue.trimmed()));
                headerName.clear();
                headerValue.clear();
                headersState = HeadersState::NameStart;
            } else {
                // Error: expected \n
                handleError(StreamError::Kind::UnexpectedCharacter);
                return;
            }
            break;
        case HeadersState::End:
            if (c != '\n') {
                // Error: expected \n after \r
                handleError(StreamError::Kind::UnexpectedCharacter);
                return;
            }

            state = State::Payload;
            initialisePayload();
    }
}

void FrameBuilder::initialisePayload() {
    buffer.clear();

    pending = getPayloadSizeFromHeaders();
    if (pending < 0) {
        return;
    }

    if (pending == 0) {
        emitPayload();
        return;
    }

    buffer.reserve(pending);
}

void FrameBuilder::appendPayload(char c) {
    buffer.append(c);
    pending -= 1;
    if (pending == 0) {
        emitPayload();
    }
}

void FrameBuilder::emitPayload() {
    emit emitFrame(Frame (QDateTime::currentMSecsSinceEpoch(), frameStart, offset, offset - buffer.size(), headers, buffer, recoveryState == 0));

    frameStart = offset;

    state = State::Headers;
    headersState = HeadersState::NameStart;
    headers.clear();

    if (recoveryState > 0) {
        recoveryState -= 1;
    }
}

long FrameBuilder::getPayloadSizeFromHeaders() {
    long length = -1;

    for (Header header : headers) {
        if (header.name.compare("Content-Length", Qt::CaseInsensitive) != 0) {
            continue;
        }

        if (length >= 0) {
            handleError(StreamError::Kind::MultipleContentLength);
            return -1;
        }

        bool success = false;
        length = header.value.toLong(&success, 10);

        if (!success) {
            handleError(StreamError::Kind::ContentLengthNaN);
            return -1;
        }

        if (length < 0) {
            handleError(StreamError::Kind::ContentLengthNegative);
            return -1;
        }
    }

    if (length >= 0) {
        return length;
    }

    // LSP defines Content-Length header as required
    handleError(StreamError::Kind::MissingContentLength);
    return -1;
}

void FrameBuilder::handleError(StreamError::Kind kind) {
    if (recoveryState == 0) {
        emit emitError(StreamError(offset, offset - frameStart, kind));
    }
    frameStart = offset;
    headers.clear();
    state = State::Headers;
    headersState = HeadersState::NameStart;
    recoveryState += 1;
}

}
