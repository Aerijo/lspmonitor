#include "lspschemavalidator.h"

#include <QException>

namespace Lsp {

ParsedMessage::ParsedMessage() : ParsedMessage(nullptr) {}
ParsedMessage::ParsedMessage(Message* message) : ParsedMessage(message, QVector<SchemaIssue>()) {}
ParsedMessage::ParsedMessage(Message* message, SchemaIssue issue) : ParsedMessage(message, QVector<SchemaIssue> { issue }) {}
ParsedMessage::ParsedMessage(Message* message, QVector<SchemaIssue> issues) : message(message), issues(issues) {}

SchemaIssue SchemaIssue::error(QString msg) {
    return SchemaIssue(SchemaIssueSeverity::Error, msg);
}

SchemaIssue::SchemaIssue(SchemaIssueSeverity severity, QString msg) : severity(severity), message(msg) {}

ParsedMessage Message::fromJson(QJsonDocument json) {
    if (!json.isObject()) {
        return ParsedMessage(nullptr, SchemaIssue::error("Message must be an object"));
    }

    QVector<SchemaIssue> issues {};

    QJsonObject root = json.object();

    int size = root.size();

    auto methodIt = root.find("method");
    if (methodIt != root.end()) {
        // Notifications and Requests have methods
        QJsonValue m = methodIt.value();

        if (!m.isString()) {
            issues.append(SchemaIssue::error("Method must be of type 'string'"));
        }

    } else {
        // Responses do not have methods


    }
}

}
