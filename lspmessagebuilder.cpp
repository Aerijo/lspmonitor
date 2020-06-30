#include "lspmessagebuilder.h"
#include <iostream>

QString lspEntityToQString(LspEntity entity) {
    switch (entity) {
    case LspEntity::Client:
        return "Client";
    case LspEntity::Server:
        return "Server";
    default:
        return "Unknown";
    }
}

void LspMessageBuilder::append(QByteArray data) {
    for (char c : data) {
        switch (state) {
        case LspMsgBuilderState::Headers:
            switch (c) {
            case '\r':
                if (pendingPayload == 0 || pendingPayload == 2) {
                    pendingPayload += 1;
                } else {
                    // Error: Unexpected \r
                    state = LspMsgBuilderState::Recovery;
                }
                break;
            case '\n':
                if (pendingPayload == 1) {
                    pendingPayload += 1;

                    QString raw = QString::fromStdString(buffer.toStdString());

                    buffer.clear();

                    size_t split = raw.indexOf(':');
                    if (split < 0) {
                        // throw header parse error
                        state = LspMsgBuilderState::Recovery;
                    } else {
                        // TODO: Parse and detect errors such as value whitespace, non ASCII
                        headers.append({ raw, raw.mid(0, split), raw.mid(split + 1, raw.size()).trimmed() });
                    }
                } else if (pendingPayload == 3) {
                    state = LspMsgBuilderState::Payload;

                    bool has_length = false;
                    for (LspHeader header : headers) {
                        if (header.keyEquals("Content-length")) {
                            QString value = header.getValue();
                            pendingPayload = value.toUInt(&has_length, 10);
                            break;
                        }
                    }

                    if (!has_length) {
                        // throw missing / malformed content length error
                        state = LspMsgBuilderState::Recovery;
                    }
                } else {
                    buffer.append(c);
                }
                break;
            default:
                pendingPayload = 0;
                buffer.append(c);
            }
            break;
        case LspMsgBuilderState::Payload:
            buffer.append(c);
            pendingPayload -= 1;
            if (pendingPayload == 0) {
                // TODO: Handle parse error
                QJsonDocument payload = QJsonDocument::fromJson(buffer);

                LspMessage *msg = new LspMessage {source, headers, payload};
                emit emitLspMessage(msg);

                buffer.clear();
                headers.clear();
                state = LspMsgBuilderState::Headers;
            }
            break;
        case LspMsgBuilderState::Recovery:
            size_t magic = 0x11FFFF;
            if (pendingPayload != magic) {
                std::cerr << "in recovery mode" << std::endl;
                pendingPayload = magic;
            }
            break;
        }
    }
}
