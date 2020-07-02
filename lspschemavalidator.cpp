#include "lspschemavalidator.h"

#include <QException>

namespace Lsp {

LspSchemaValidator::LspSchemaValidator(LspMessage::Sender sender, QObject* parent) : QObject(parent), sender(sender) {}

SchemaIssue::SchemaIssue(Severity severity, QString msg) : severity(severity), message(msg) {}

SchemaIssue SchemaIssue::error(QString msg) {
    return SchemaIssue(Severity::Error, msg);
}

SchemaJson::SchemaJson() : SchemaJson(Kind::Empty) {}

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
        case Kind::Empty:
            break;
    }
}

SchemaJson::SchemaJson(const SchemaJson& other) {
    localIssues = other.localIssues;
    kind = other.kind;
    switch (other.kind) {
        case Kind::Object:
            mapProperties = other.mapProperties;
            break;
        case Kind::Array:
            arrayValues = other.arrayValues;
            break;
        case Kind::Empty:
            break;
    }
}

void SchemaJson::intoObject() {
    if (kind == Kind::Object) {
        return;
    }

    if (kind != Kind::Empty) {
        throw QUnhandledException();
    }

    kind = Kind::Object;
    mapProperties = {};
}

void SchemaJson::intoArray() {
    if (kind == Kind::Array) {
        return;
    }

    if (kind != Kind::Empty) {
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

int SchemaJson::issueCount() const {
    int count = localIssues.size();

    if (isObject()) {
        for (auto it = mapProperties.begin(); it != mapProperties.end(); it++) {
            count += it->second.first.size();
            count += it->second.second.issueCount();
        }
    } else if (isArray()) {
        for (auto child : arrayValues) {
            count += child.issueCount();
        }
    }

    return count;
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

    // 1. Assert "jsonrpc" property is present + has value "2.0"
    auto it = contents.find("jsonrpc");
    if (it == contents.end()) {
        issues.error("'jsonrpc' member missing");
    } else if (!it.value().isString()) {
        issues.keyError("jsonrpc", "Expected value to be the string \"2.0\"");
    } else if (it.value().toString().compare("2.0") != 0) {
        issues.member("jsonrpc").error("Expected value to be \"2.0\"");
    }

    // 2. Detect what kind of message it is
    auto kind = LspMessage::Kind::Unknown;

    auto methodIt = contents.find("method");
    auto idIt = contents.find("id");

    if (methodIt != contents.end()) {
        option<QString> method;
        option<QJsonDocument> params {};

        if (!methodIt.value().isString()) {
            issues.keyError("method", "Expected method to be a string");
        } else {
            method = methodIt.value().toString();
        }

        if (idIt != contents.end()) {
            kind = LspMessage::Kind::Request;
            if (!(idIt.value().isString() || idIt.value().isDouble())) {
                issues.keyError("id", "Expected id to be a string or number");
            }
        } else {
            kind = LspMessage::Kind::Notification;
        }

        for (auto it = contents.begin(); it != contents.end(); it++) {
            if (it.key() == "jsonrpc" || it.key() == "method" || it.key() == "id") {
                continue;
            }

            if (it.key() == "params") {
                if (it.value().isObject()) {
                    params = QJsonDocument(it.value().toObject());
                } else if (it.value().isArray()) {
                    params = QJsonDocument(it.value().toArray());
                } else {
                    issues.keyError("params", "Expected params to be an object or array");
                }

                continue;
            }

            issues.keyError(it.key(), "Unexpected method '" + it.key() + "'");
        }

        if (method) {
            if (kind == LspMessage::Kind::Notification) {
                validateNotification(method.value(), params, issues);
            } else {
                validateRequest(method.value(), params, issues);
            }
        }

    } else if (idIt != contents.end()) {
        kind = LspMessage::Kind::Response;

        if (!(idIt.value().isString() || idIt.value().isDouble())) {
            issues.keyError("id", "Expected id to be a string or number");
        }

        bool isSuccess = false;
        bool isError = false;

        for (auto it = contents.begin(); it != contents.end(); it++) {
            if (it.key() == "jsonrpc" || it.key() == "id") {
                continue;
            }

            if (it.key() == "result") {
                isSuccess = true;
                continue;
            }

            if (it.key() == "error") {
                isError = true;
                continue;
            }

            issues.keyError(it.key(), "Unexpected method '" + it.key() + "'");
        }

        if (isSuccess) {
            if (isError) {
                issues.keyError("error", "'error' method not permitted when there is a result");
            }
        } else if (isError) {
            validateResponseError(contents.find("error").value(), issues);
        } else {
            issues.error("'result' or 'error' method required on Response");
        }

    } else {
        issues.error("Could not identify message kind");
        return;
    }

    qDebug() << QString::number(issues.issueCount());

    // Then specialise the validation based on kind + method (if notification / request)

    auto lspMsg = std::make_shared<LspMessage>(kind, issues, message);
    lspMsg->sender = sender;

    emitLspMessage(lspMsg);
}

void LspSchemaValidator::validateResponseError(QJsonValue errorMethod, SchemaJson &rootIssues) {
    if (!errorMethod.isObject()) {
        rootIssues.keyError("error", "'error' method must be an object");
        return;
    }

    QJsonObject err = errorMethod.toObject();
    SchemaJson errIssues = rootIssues.member("error");

    bool hasCode = false;
    bool hasMessage = false;

    for (auto it = err.begin(); it != err.end(); it++) {
        if (it.key() == "code") {
            hasCode = true;

            // TODO: Replace Json implementation with one that actually lets you work with numbers precisely
            // with QJson, we can't even correctly tell if the number is an integer or float.
            if (!it.value().isDouble() || it.value().toDouble() != std::floor(it.value().toDouble())) {
                errIssues.keyError("code", "The 'code' member must be an integer");
            } else {
                switch (it.value().toInt()) {
                    case -32700:
                    case -32600:
                    case -32601:
                    case -32602:
                    case -32603:
                    case -32099:
                    case -32000:
                    case -32002:
                    case -32001:
                    case -32800:
                    case -32801:
                        break;
                    default:
                        errIssues.member("code").error("Error code not recognised");
                }
            }


        } else if (it.key() == "message") {
            hasMessage = true;

            if (!it.value().isString()) {
                errIssues.keyError("message", "Error message must be a string");
            }

        } else if (it.key() != "data") { // data can be anything, or even omitted
            errIssues.keyError(it.key(), "Unexpected member '" + it.key() + "'");
        }
    }

    if (!hasCode) {
        errIssues.error("'code' member required on Response error");
    }

    if (!hasMessage) {
        errIssues.error("'message' member required on Response error");
    }
}

void LspSchemaValidator::validateNotification(QString method, option<QJsonDocument> params, SchemaJson &issues) {
    issues.error("I don't like notifications");
}

void LspSchemaValidator::validateRequest(QString method, option<QJsonDocument> params, SchemaJson &issues) {

}

LspMessage::LspMessage(MessageBuilder::Message msg) : LspMessage(Kind::Unknown, SchemaJson::makeObject(), msg) {}

LspMessage::LspMessage(LspMessage::Kind kind, SchemaJson issues, MessageBuilder::Message msg) : kind(kind), issues(issues), timestamp(msg.timestamp), contents(msg.contents) {}

}
