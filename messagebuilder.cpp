#include "messagebuilder.h"
#include "asciiparsing.h"

#include <QTextCodec>
#include <QJsonValue>

namespace MessageBuilder {

Message::Message(FrameBuilder::Frame frame, QJsonDocument contents) : Message(frame.timestamp, contents, frame.frameEnd - frame.frameStart) {}
Message::Message(qint64 timestamp, QJsonDocument contents, int size) : timestamp(timestamp), contents(contents), size(size) {}


HeaderParser::HeaderParser(QString value) : value(value) {}

QString HeaderParser::parse(char c) {
    if (index >= value.length() || value.at(index) != c) {
        setError(index);
        return "";
    }

    index += 1;
    return QString (c);
}

QString HeaderParser::parse(Kind kind) {
    int i = index;
    QString result;

    switch (kind) {
        case Kind::Token:
            for (; i < value.size(); i++) {
                if (!isTchar(value.at(i))) {
                    break;
                }
            }

            if (i == index) {
                setError(index);
                return {};
            }

            result = value.mid(index, i - index);
            break;
        case Kind::OptionalWhitespace:
        case Kind::MandatoryWhitespace:
            for (; i < value.size(); i++) {
                if (!isHorizontalWhitespace(value.at(i))) {
                    break;
                }
            }
            if (kind != Kind::OptionalWhitespace && i == index) {
                setError(index);
                return {};
            }

            result = value.mid(index, i - index);
            break;
        case Kind::ParamValue:
            if (index >= value.size()) {
                setError(index);
                return {};
            }

            if (value.at(index) == '"') {
                return parse(Kind::QuotedString);
            } else {
                return parse(Kind::Token);
            }
            break;
        case Kind::QuotedString:
            if (index >= value.size() || value.at(index) != '"') {
                setError(index);
                return {};
            }

            i += 1;
            bool success = false;

            for (; i < value.size(); i++) {
                QChar c = value.at(i);

                if (c == '"') {
                    i++;
                    success = true;
                    break;
                }

                if (c == '\\' && i + 1 < value.size()) {
                    i++;
                    c = value.at(i);
                    if (!isHorizontalWhitespace(c) && !isVchar(c)) {
                        setError(i);
                        return {};
                    }
                    result += c;
                } else if (isHorizontalWhitespace(c) || c == 0x21 || (0x23 <= c && c <= 0x5B) || (0x5D <= c && c <= 0x7E)) {
                    result += c;
                } else {
                    success = false;
                    break;
                }
            }

            if (!success) {
                setError(i);
                return {};
            }

            break;
    }

    index = i;
    return result;
}

bool HeaderParser::finished() {
    return index == value.size();
}

void HeaderParser::setError(int location) {
    if (errorIndex < 0) {
        errorIndex = location;
    }
}

bool HeaderParser::errorOccurred() {
    return errorIndex > -1;
}

option<ContentTypeHeader> ContentTypeHeader::fromHeaderValue(QString value) {
    HeaderParser parser (value);

    QString type = parser.parse(HeaderParser::Kind::Token);
    parser.parse('/');
    QString subtype = parser.parse(HeaderParser::Kind::Token);

    if (parser.errorOccurred()) {
        return {};
    }

    ContentTypeHeader ctHeader {type, subtype};

    if (parser.finished()) {
        return ctHeader;
    }

    while (!parser.finished()) {
        parser.parse(HeaderParser::Kind::OptionalWhitespace);
        parser.parse(';');
        if (!parser.errorOccurred()) {
            return {};
        }
        parser.parse(HeaderParser::Kind::OptionalWhitespace);
        QString name = parser.parse(HeaderParser::Kind::Token);
        parser.parse('=');

        QString value = parser.parse(HeaderParser::Kind::ParamValue);

        if (parser.errorOccurred()) {
            return {};
        }

        ctHeader.parameters.append(std::pair<QString, QString>(name, value));
    }

    return ctHeader;
}

MessageBuilder::MessageBuilder(QObject *parent) : QObject(parent) {}


/**
 * We just look for possible values, indicating invalid values/syntax can be
 * implemented some other time.
 */
QString getEncoding(const QVector<FrameBuilder::Header> &headers) {
    for (FrameBuilder::Header header : headers) {
        if (header.name.compare("Content-Type") != 0) {
            continue;
        }

        option<ContentTypeHeader> value = ContentTypeHeader::fromHeaderValue(header.value);
        if (!value) {
            break;
        }

        for (auto param : value.value().parameters) {
            if (param.first.compare("charset") == 0) {
                return param.second;
            }
        }
    }

    return "UTF-8";
}

void MessageBuilder::onFrame(FrameBuilder::Frame frame) {
    QString encoding = getEncoding(frame.headers);

    QByteArray jsonInput = frame.payload;

    if (encoding.compare("UTF-8", Qt::CaseInsensitive) != 0) {
        QTextCodec *codec = QTextCodec::codecForName(encoding.toUtf8());
        if (codec == nullptr) {
            emitError();
            return;
        }
        jsonInput = codec->toUnicode(jsonInput).toUtf8();
    }

    QJsonParseError error;

    // TODO: Does this detect duplicate object keys? (if not, we need to find them and raise an error on them)
    QJsonDocument doc = QJsonDocument::fromJson(jsonInput, &error);

    if (error.error == QJsonParseError::NoError) {

        // NOTE: JSON-RPC 2.0 uses RFC 4627 for JSON definition,
        // which specifies the root as being an object or array
        // (matched by Qt parsing rules). So we don't bother
        // trying to support other kinds of root value.

        emit emitMessage(Message(frame, doc));
    } else {
        emit emitError();
    }
}

}
