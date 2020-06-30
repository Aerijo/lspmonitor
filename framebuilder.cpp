#include "framebuilder.h"

namespace FrameBuilder {

Header::Header(QString name, QString value) : name(name), value(value) {}

Frame::Frame(size_t gStart, size_t gEnd, size_t pOff, QList<Header> headers, QByteArray payload, bool recovery) : frameStart(gStart), frameEnd(gEnd), payloadStart(pOff), headers(headers), payload(payload), fromRecoveryMode(recovery) {}

StreamError::StreamError(size_t gOffset, size_t lOffset, Kind kind) : globalOffset(gOffset), localOffset(lOffset), kind(kind) {}

FrameBuilder::FrameBuilder(QObject* parent) : QObject(parent) {}

void FrameBuilder::operator<<(const QByteArray &data) {
    for (char c : data) {
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

void FrameBuilder::operator<<(char c) {
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

bool isVchar(char c) {
    return 0x21 <= c && c <= 0x7E;
}

bool isDelim(char c) {
    switch (c) {
        case '(':
        case ')':
        case ',':
        case '/':
        case ':':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
        case '@':
        case '[':
        case '\\':
        case ']':
        case '{':
        case '}':
            return true;
        default:
            return false;
    }
}

bool isHorizontalWhitespace(char c) {
    return c == ' ' || c == '\t';
}

bool isTchar(char c) {
    return isVchar(c) && !isDelim(c);
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
    emit emitFrame(Frame (frameStart, offset, offset - buffer.size(), headers, buffer, recoveryState == 0));

    frameStart = offset;

    state = State::Headers;

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
}

}
