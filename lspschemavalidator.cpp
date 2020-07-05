#include "lspschemavalidator.h"

#include <QException>

namespace Lsp {

Version::Version(int major, int minor, int patch) : major(major), minor(minor), patch(patch) {}

Id::Id() : kind(Kind::Invalid) {}
Id::Id(int id) : kind(Kind::Number), numberId(id) {}
Id::Id(QString id) : kind(Kind::String), stringId(id) {}
bool Id::isValid() const { return kind != Kind::Invalid; }
bool Id::isNumber() const { return kind == Kind::Number; }
bool Id::isString() const { return kind == Kind::String; }
int Id::getNumber() const { return numberId; }
QString Id::getString() const { return stringId; }

Context::Context(MessageBuilder::Message message, Entity sender) : Context(message.timestamp, sender, message.contents, message.size) {}
Context::Context(qint64 timestamp, Entity sender, QJsonDocument contents, int size) : timestamp(timestamp), sender(sender), contents(contents), size(size) {}

Message::Message(Context c) : sender(c.sender), timestamp(c.timestamp), size(c.size) {}
SchemaJson* Message::getIssues() { return issues.get(); }
Entity Message::getSender() const { return sender; }
qint64 Message::getTimestamp() const { return timestamp; }
void Message::setIndex(int index) { this->index = index; }
int Message::getIndex() const { return index; }
int Message::getIssueCount() const {
    if (issues) {
        return issues->issueCount();
    } else {
        return 0;
    }
}
int Message::getSize() const { return size; }

GenericMessage::GenericMessage(Context c) : Message(c), contents(c.contents) {}
option<QString> GenericMessage::tryGetMethod() const { return {}; }
Message::Kind GenericMessage::getKind() const { return Kind::Unknown; }
QJsonDocument GenericMessage::getContents() const { return contents; }

Notification::Notification(Context c, QString method) : Message(c), method(method) {}
Message::Kind Notification::getKind() const { return Kind::Notification; }
option<QString> Notification::tryGetMethod() const { return getMethod(); }
QString Notification::getMethod() const { return method; }

GenericNotification::GenericNotification(Context c, QString method) : Notification(c, method), contents(c.contents) {}
QJsonDocument GenericNotification::getContents() const { return contents; }

Request::Request(Context c, QString method, Id id) : Message(c), method(method), id(id) {}
Message::Kind Request::getKind() const { return Kind::Request; }
option<QString> Request::tryGetMethod() const { return getMethod(); }
QString Request::getMethod() const { return method; }
Id Request::getId() const { return id; }
std::shared_ptr<Response> Request::getResponse() { return response; }
void Request::setResponse(std::shared_ptr<Response> response) { this->response = response; }

GenericRequest::GenericRequest(Context c, QString method, Id id) : Request(c, method, id), contents(c.contents) {}
QJsonDocument GenericRequest::getContents() const { return contents; }

Response::Response(Context c, Id id) : Message(c), id(id) {}
Message::Kind Response::getKind() const { return Kind::Response; }
Id Response::getId() const { return id; }
std::shared_ptr<Request> Response::getRequest() { return request; }
void Response::setRequest(std::shared_ptr<Request> request) { this->request = request; }
option<QString> Response::tryGetMethod() const {
    if (!request) {
        return {};
    }
    return request->getMethod();
}
qint64 Response::getDuration() const {
    if (!request) {
        return -1;
    }

    return getTimestamp() - request->getTimestamp();
}

GenericResponse::GenericResponse(Context c, Id id) : Response(c, id), contents(c.contents) {}
QJsonDocument GenericResponse::getContents() const { return contents; }

LspSchemaValidator::LspSchemaValidator(Lsp::Entity sender, QObject* parent) : QObject(parent), sender(sender) {}

SchemaIssue::SchemaIssue(Severity severity, QString msg) : severity(severity), message(msg) {}
SchemaIssue SchemaIssue::error(QString msg) { return SchemaIssue(Severity::Error, msg); }

SchemaJson::SchemaJson() : SchemaJson(Kind::Empty) {}
SchemaJson SchemaJson::makeObject() { return SchemaJson(Kind::Object); }
SchemaJson SchemaJson::makeArray() { return SchemaJson(Kind::Array); }
bool SchemaJson::isObject() const { return kind == Kind::Object; }
bool SchemaJson::isArray() const { return kind == Kind::Array; }

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
        Context c (message, sender);
        auto lsp = std::make_shared<GenericMessage>(c);
        lsp->getIssues()->error("Unexpected message JSON type");
        emit emitLspMessage(lsp);
    }
}

void LspSchemaValidator::onMessageBatch(MessageBuilder::Message message, QJsonArray batch) {
    // TODO: Add support
    throw QUnhandledException();
    // Batched messages, for now just turn them into individual messages
//    for (QJsonValue entry : batch) {
//        if (entry.isObject()) {
//            onMessage(MessageBuilder::Message(message.timestamp, QJsonDocument(entry.toObject())));
//        } else {
//            // Note: Fallthrough / recursive call not used because constructing a meaningful Message with the
//            // QJsonDocument incompatible value is difficult. Also it may cause a child array to be interpreted
//            // as a batch itself, which is forbidden
////            auto errMsg = std::make_shared<LspMessage>(message);
////            errMsg->issues.error("Unexpected message JSON type");
////            emitLspMessage(errMsg);
//            throw QUnhandledException();
//        }
//    }
}

void LspSchemaValidator::onMessageObject(MessageBuilder::Message message, QJsonObject contents) {
    std::shared_ptr<Message> result;

    Context c (message, sender);

    // Ensure "jsonrpc" member correct
    validateJsonrpcMember(c, contents);

    // Detect what kind of message it is
    auto &issues = c.issues;

    auto methodIt = contents.find("method");
    auto idIt = contents.find("id");

    option<QString> method;
    option<QJsonDocument> params {};
    option<Id> id;

    if (methodIt.value().isString()) {
        method = methodIt.value().toString();
    } else {
        issues.keyError("method", "Expected method to be a string");
    }

    if (idIt != contents.end()) {
        if (idIt.value().isString()) {
            id = idIt.value().toString();
        } else if (idIt.value().isDouble()) {
            id = idIt.value().toInt();
        } else {
            issues.keyError("id", "Expected id to be a string or number");
            id = Id();
        }
    }

    for (auto it = contents.begin(); it != contents.end(); it++) {
        if (it.key() == "jsonrpc" || it.key() == "method" || it.key() == "id" || it.key() == "params") {
            continue;
        }

        issues.keyError(it.key(), "Unexpected member '" + it.key() + "'");
    }

    if (method) {
        if (id) {
            // method + id == Request
            result = buildRequest(c, method.value(), id.value());
        } else {
            // method == Notification
            result = buildNotification(c, method.value());
        }
    } else if (id) {
        // id == Response
        result = buildResponse(c, id.value());
    } else {
        issues.error("Could not identify message kind");
        result = std::make_shared<GenericMessage>(c);
    }

    emitLspMessage(result);
}

void LspSchemaValidator::validateJsonrpcMember(Context &c, const QJsonObject &contents) {
    auto it = contents.find("jsonrpc");
    if (it == contents.end()) {
        c.issues.error("'jsonrpc' member missing");
    } else if (!it.value().isString()) {
        c.issues.keyError("jsonrpc", "Expected value to be the string \"2.0\"");
    } else if (it.value().toString().compare("2.0") != 0) {
        c.issues.member("jsonrpc").error("Expected value to be \"2.0\"");
    }
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
//    issues.error("I don't like notifications");
}

void LspSchemaValidator::validateRequest(option<Id> id, QString method, option<QJsonDocument> params, SchemaJson &issues, std::shared_ptr<LspMessage> msg) {
//    if (id) {
//        auto existing = idTracker.insert(id.value(), msg);
//        if (existing) {
//            issues.member("id").error("This ID value is already in use for an existing request (" + existing->contents["method"].toString() + ")");
//        }
//    }
}

void LspSchemaValidator::validateResponseSuccess(Id id, QJsonValue result, SchemaJson &issues, std::shared_ptr<LspMessage> msg) {

}

std::shared_ptr<Notification> LspSchemaValidator::buildNotification(Context c, QString method) {
    return std::make_unique<GenericNotification>(c, method);

//    if (it.key() == "params") {
//        if (it.value().isObject()) {
//            params = QJsonDocument(it.value().toObject());
//        } else if (it.value().isArray()) {
//            params = QJsonDocument(it.value().toArray());
//        } else {
//            issues.keyError("params", "Expected params to be an object or array");
//        }

//        continue;
//    }
}

std::shared_ptr<Request> LspSchemaValidator::buildRequest(Context c, QString method, Id id) {
    auto result = std::make_shared<GenericRequest>(c, method, id);

    auto existing = idTracker.insert(id, result);
    if (existing) {
        c.issues.member("id").error("ID already in use");
    }

    return result;
}

std::shared_ptr<Response> LspSchemaValidator::buildResponse(Context c, Id id) {
    auto response = std::make_shared<GenericResponse>(c, id);

    auto request = idTracker.retrieve(id);
    if (request) {
        response->setRequest(request.value());

        // TODO: Alert model that the response has changed / handle in model
        request.value()->setResponse(response);
    } else {
        c.issues.member("id").error("ID does not correspond to any pending Request");
    }

    return response;

//    auto match = idTracker.retrieve(id.value());
//    if (!match) {
//        issues.member("id").error("Unknown ID");
//    } else {
//        lspMsg->method = match->method;
//        lspMsg->match = match;
//        match->match = lspMsg;
//    }



//    bool isSuccess = false;
//    bool isError = false;

//    for (auto it = contents.begin(); it != contents.end(); it++) {
//        if (it.key() == "jsonrpc" || it.key() == "id") {
//            continue;
//        }

//        if (it.key() == "result") {
//            isSuccess = true;
//            continue;
//        }

//        if (it.key() == "error") {
//            isError = true;
//            continue;
//        }

//        issues.keyError(it.key(), "Unexpected method '" + it.key() + "'");
//    }

//    if (isSuccess) {
//        if (isError) {
//            issues.keyError("error", "'error' method not permitted when there is a result");
//        } else if (id) {
//            validateResponseSuccess(id.value(), contents.find("result").value(), issues, lspMsg);
//        }
//    } else if (isError) {
//        validateResponseError(contents.find("error").value(), issues);
//    } else {
//        issues.error("'result' or 'error' method required on Response");
//    }
}

template <typename T>
option<T> IdTracker<T>::insert(Id id, T msg) {
    option<T> ret {};

    if (id.isNumber()) {
        auto it = numberIds.find(id.getNumber());
        if (it != numberIds.end()) {
            ret = it.value();
        }
        numberIds[id.getNumber()] = msg;
    } else if (id.isString()) {
        auto it = stringIds.find(id.getString());
        if (it != stringIds.end()) {
            ret = it.value();
        }
        stringIds[id.getString()] = msg;
    } else {
        throw QUnhandledException();
    }

    return ret;
}

template <typename T>
option<T> IdTracker<T>::retrieve(Id id) {
    option<T> ret {};

    if (id.isNumber()) {
        auto it = other->numberIds.find(id.getNumber());
        if (it != other->numberIds.end()) {
            ret = it.value();
            other->numberIds.erase(it);
        }
    } else if (id.isString()) {
        auto it = other->stringIds.find(id.getString());
        if (it != other->stringIds.end()) {
            ret = it.value();
            other->stringIds.erase(it);
        }
    }

    return ret;
}

template <typename T>
void IdTracker<T>::linkWith(IdTracker<T> &other) {
    this->other = &other;
    other.other = this;
}

void LspSchemaValidator::linkWith(LspSchemaValidator &other) {
    idTracker.linkWith(other.idTracker);
}

LspMessage::LspMessage(MessageBuilder::Message msg) : LspMessage(Kind::Unknown, SchemaJson::makeObject(), msg) {}

LspMessage::LspMessage(LspMessage::Kind kind, SchemaJson issues, MessageBuilder::Message msg) : kind(kind), issues(issues), timestamp(msg.timestamp), contents(msg.contents) {}

}
