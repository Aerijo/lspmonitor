#include "lspschemavalidator.h"

#include <QException>

namespace Lsp {

LspSchemaValidator::LspSchemaValidator(QObject* parent) : QObject(parent) {}

SchemaIssue::SchemaIssue(Severity severity, QString msg) : severity(severity), message(msg) {}

SchemaIssue SchemaIssue::error(QString msg) {
    return SchemaIssue(Severity::Error, msg);
}

SchemaJson::SchemaJson() : SchemaJson(Kind::Value) {}

SchemaJson SchemaJson::makeObject() {
    return SchemaJson(Kind::Object);
}

SchemaJson SchemaJson::makeArray() {
    return SchemaJson(Kind::Array);
}

SchemaJson SchemaJson::member(QString key) {
    if (kind != Kind::Object) {
        throw QUnhandledException();
    }

    return mapProperties[key].second;
}

SchemaJson::SchemaJson(Kind kind) {
    this->kind = kind;
    switch (kind) {
        case Kind::Object:
            mapProperties = {};
            break;
        case Kind::Array:
            arrayValues = {};
            break;
        case Kind::Value:
            break;
    }
}

SchemaJson::SchemaJson(const SchemaJson& other) {
    kind = other.kind;
    switch (other.kind) {
        case Kind::Object:
            mapProperties = other.mapProperties;
            break;
        case Kind::Array:
            arrayValues = other.arrayValues;
            break;
        case Kind::Value:
            break;
    }
}

SchemaJson::~SchemaJson() {
    localIssues.~QVector();

    switch (kind) {
        case Kind::Object:
            mapProperties.~map();
            break;
        case Kind::Array:
            arrayValues.~QVector();
            break;
        case Kind::Value:
            break;
    }
}

void SchemaJson::intoObject() {
    if (kind == Kind::Object) {
        return;
    }

    if (kind != Kind::Value) {
        throw QUnhandledException();
    }

    kind = Kind::Object;
    mapProperties = {};
}

void SchemaJson::intoArray() {
    if (kind == Kind::Array) {
        return;
    }

    if (kind != Kind::Value) {
        throw QUnhandledException();
    }

    kind = Kind::Array;
    arrayValues = {};
}

void SchemaJson::error(QString msg) {
    localIssues.append(SchemaIssue::error(msg));
}

void SchemaJson::keyError(QString key, QString msg) {
    if (kind != Kind::Object) {
        throw QUnhandledException();
    }

    mapProperties[key].first.append(SchemaIssue::error(msg));
}

void LspSchemaValidator::onMessage(MessageBuilder::Message message) {
    QJsonDocument root = message.contents;

    if (root.isArray()) {
        onMessageBatch(message, root.array());
    } else if (root.isObject()) {
        onMessageObject(message, root.object());
    } else {
        auto lsp = std::make_shared<LspMessage>(message);
        lsp->issues.error("Unexpected message JSON type");
        emit emitLspMessage(lsp);
    }
}

void LspSchemaValidator::onMessageBatch(MessageBuilder::Message message, QJsonArray batch) {
    // Batched messages, for now just turn them into individual messages
    for (QJsonValue entry : batch) {
        if (entry.isObject()) {
            onMessage(MessageBuilder::Message(message.timestamp, QJsonDocument(entry.toObject())));
        } else {
            // Note: Fallthrough / recursive call not used because constructing a meaningful Message with the
            // QJsonDocument incompatible value is difficult. Also it may cause a child array to be interpreted
            // as a batch itself, which is forbidden
            auto errMsg = std::make_shared<LspMessage>(message);
            errMsg->issues.error("Unexpected message JSON type");
            emitLspMessage(errMsg);
        }
    }
}

void LspSchemaValidator::onMessageObject(MessageBuilder::Message message, QJsonObject contents) {
    SchemaJson issues = SchemaJson::makeObject();

    // 1. Assert "jsonrpc" property is present + has correct value
    auto it = contents.find("jsonrpc");
    if (it == contents.end()) {
        issues.error("'jsonrpc' member missing");
    } else if (!it.value().isString()) {
        issues.keyError("jsonrpc", "Expected value to be the string \"2.0\"");
    } else if (it.value().toString().compare("2.0") != 0) {
        issues.member("jsonrpc").error("Expected value to be \"2.0\"");
    }

    // 2. Detect what kind of message it is
    auto methodIt = contents.find("method");
    if (methodIt != contents.end()) {

        if (!methodIt.value().isString()) {
            issues.keyError("method", "Expected method to be a string");
        }

        // Notification or Request, depending on "id" member
        auto idIt = contents.find("id");
        if (idIt != contents.end()) {
            // Request

            if (!(idIt.value().isString() || idIt.value().isDouble())) {
                issues.keyError("id", "Expected id to be a string or number");
            }


        } else {
            // Notification
        }
    } else {
        // Response

    }

    // Then specialise the validation based on kind + method (if notification / request)

    auto lspMsg = std::make_shared<LspMessage>(LspMessage::Kind::Unknown, std::move(issues), message);

    emitLspMessage(lspMsg);
}

LspMessage::LspMessage(MessageBuilder::Message msg) : LspMessage(Kind::Unknown, SchemaJson::makeObject(), msg) {}

LspMessage::LspMessage(LspMessage::Kind kind, SchemaJson issues, MessageBuilder::Message msg) : kind(kind), issues(issues), timestamp(msg.timestamp), contents(msg.contents) {}

}
