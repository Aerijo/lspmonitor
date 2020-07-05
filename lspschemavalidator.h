#ifndef LSPSCHEMAVALIDATOR_H
#define LSPSCHEMAVALIDATOR_H

#include <QJsonDocument>
#include <QJsonObject>

#include <map>

#include "option.h"
#include "messagebuilder.h"

namespace Lsp {

/**
 * Represents an LSP version. Used for cases like "introduced in version",
 * or "deprecated in version", etc.
 */
struct Version {
    int major;
    int minor;
    int patch;

    Version(int major, int minor, int patch);
};

/**
 * URI that refers to a document. Encoded as a string,
 * but URI structure expected.
 */
class DocumentUri {
    QString raw;
};

/**
 * Represents an LSP Id (number or string)
 */
class Id {
public:
    Id();
    Id(int id);
    Id(QString id);

    bool isValid() const;
    bool isNumber() const;
    bool isString() const;

    int getNumber() const;
    QString getString() const;

private:
    enum class Kind {
        String,
        Number,
        Invalid,
    } kind;

    QString stringId {};
    int numberId {};
};

enum class Entity {
    Client,
    Server,
};

enum class ErrorCodes {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ServerErrorStart = -32099,
    ServerErrorEnd = -32000,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001,

    RequestCancelled = -32800,
    ContentModified = -32801,
};

enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4,

    /** Represents values that are not part of the specification (may be present in homebrew extensions) */
    Other,
};

enum class DiagnosticTag {
    Unnecessary = 1,
    Deprecated = 2,

    /** Represents values that are not part of the specification (may be present in homebrew extensions) */
    Other,
};

enum class ResourceOperationKind {
    Create,
    Rename,
    Delete,

    Other,
};

enum class FailureHandlingKind {
    Abort,
    Transactional,
    TextOnlyTransactional,
    Undo,

    Other,
};

enum class MarkupKind {
    PlainText,
    Markdown,

    Other,
};

enum class Trace {
    Off,
    Messages,
    Verbose,

    Other,
};

enum class InitializeError {
    UnknownProtocolVersion = 1,

    Other,
};

struct SchemaIssue {
    enum class Severity {
        Error = 1,
        Warning = 2,
        Info = 3,
    } severity;

    QString message;

    static SchemaIssue error(QString msg);

    SchemaIssue(Severity severity, QString message);
};

/**
 * JSON like tree structure that is a subset of the message, with
 * entries nesting to reach all schema issues
 */
class SchemaJson {
public:
    QVector<SchemaIssue> localIssues;

    bool isObject() const;

    bool isArray() const;

    void error(QString msg);

    void keyError(QString key, QString msg);

    void intoObject();

    void intoArray();

    int issueCount() const;

    int hintCount() const;

    int warningCount() const;

    int errorCount() const;

    SchemaJson member(QString key);

    static SchemaJson makeObject();

    static SchemaJson makeArray();

    static SchemaJson makeValue();

    SchemaJson();

    SchemaJson(const SchemaJson&);

private:
    enum class Kind {
        Object,
        Array,
        Empty,
    } kind;

    // TODO: Make union without segfaulting
    std::map<QString, std::pair<QVector<SchemaIssue>, SchemaJson>> mapProperties;
    QVector<SchemaJson> arrayValues;

    SchemaJson(Kind kind);
};

/**
 * Represents a JSON document, annotated with JSON-RPC / LSP specific
 * information such as message kind, schema violations, etc.
 */
struct LspMessage {
    /** Which entity sent the message */
    Entity sender;

    /** What kind of JSON-RPC message this is */
    enum class Kind {
        /** The message is a Notification (no acknowlegement required) */
        Notification,

        /** The message is a request (expects a response) */
        Request,

        /** The message is a response (matches with previous request) */
        Response,

        /** The message is a list of messages send in a single array */
        Batch, // TODO: Support this. For now, just split into individual messages

        /** The message does not conform to the set of known kinds */
        Unknown,
    } kind;

    /** Any issues with the message */
    SchemaJson issues;

    /** The time the message completed arriving */
    qint64 timestamp;

    /** The JSON representation of the message */
    QJsonDocument contents;

    /** The method associated with this message (the method of the Request if a Response) */
    QString method;

    /** A unique identifier, also the index of the message in the Model */
    int index = -1;

    // TODO: Remove & turn message kinds into sub classes
    std::shared_ptr<LspMessage> match;

    LspMessage() = default;

    LspMessage(MessageBuilder::Message msg);

    LspMessage(Kind kind, SchemaJson issues, MessageBuilder::Message msg);
};

struct Context {
    qint64 timestamp;
    Entity sender;
    QJsonDocument contents;
    SchemaJson issues = SchemaJson::makeObject();
    int size;

    Context(MessageBuilder::Message msg, Entity sender);
    Context(qint64 timestamp, Entity sender, QJsonDocument contents, int size);
};

/**
 * Represents a generic message derived from a Frame payload
 * that could be parsed as JSON. Messages are subclassed by their type,
 * such that the more the message conforms to a known kind the more
 * precise the type (and therefore compact the store).
 */
class Message {
public:
    /** What kind of JSON-RPC message this is */
    enum class Kind {
        /** The message is a Notification (no acknowlegement required) */
        Notification,

        /** The message is a request (expects a response) */
        Request,

        /** The message is a response (matches with previous request) */
        Response,

        /** The message is a list of messages send in a single array */
        Batch, // TODO: Support this. For now, just split into individual messages

        /** The message does not conform to the set of known kinds */
        Unknown,
    };

    Entity getSender() const;

    qint64 getTimestamp() const;

    int getIndex() const;

    void setIndex(int index);

    SchemaJson* getIssues();

    int getIssueCount() const;

    int getSize() const;

    virtual option<QString> tryGetMethod() const = 0;

    virtual Kind getKind() const = 0;

    virtual QJsonDocument getContents() const = 0;

    explicit Message(Context c);

private:
    /** Who sent this message */
    Entity sender;

    /** When the message was fully received */
    qint64 timestamp;

    /** Any issues with the message's LSP compliance (nullptr if none) */
    std::unique_ptr<SchemaJson> issues;

    /** The index of the message in the overall sequence */
    int index = -1;

    int size = -1;
};

/**
 * Any message that doesn't fall under a more specific class. Generally
 * because it's shape is incorrect (no "method" member or "id" member).
 *
 * For now includes because "jsonrpc" member incorrect, but this may
 * be moved to an issues value.
 */
class GenericMessage : public Message {
public:
    GenericMessage(Context c);

    option<QString> tryGetMethod() const override;

    Kind getKind() const override;

    QJsonDocument getContents() const override;

private:
    QJsonDocument contents;
};

class Notification : public Message {
public:
    Notification(Context c, QString method);

    Kind getKind() const override;

    option<QString> tryGetMethod() const override;

    QString getMethod() const;

private:
    QString method;
};

class GenericNotification : public Notification {
public:
    GenericNotification(Context c, QString method);

    QJsonDocument getContents() const override;

private:
    QJsonDocument contents;
};

class Response;

class Request : public Message {
public:
    Request(Context c, QString method, Id id);

    Kind getKind() const override;

    option<QString> tryGetMethod() const override;

    QString getMethod() const;

    Id getId() const;

    std::shared_ptr<Response> getResponse();

    void setResponse(std::shared_ptr<Response> response);

private:
    std::shared_ptr<Response> response;

    QString method;

    Id id;
};

class GenericRequest : public Request {
public:
    GenericRequest(Context c, QString method, Id id);

    QJsonDocument getContents() const override;

private:
    QJsonDocument contents;
};

class Response : public Message {
public:
    Response(Context c, Id id);

    Kind getKind() const override;

    Id getId() const;

    option<QString> tryGetMethod() const override;

    /** Get the duration between the original Requeat and this Response */
    qint64 getDuration() const;

    std::shared_ptr<Request> getRequest();

    void setRequest(std::shared_ptr<Request> request);

private:
    Id id;

    std::shared_ptr<Request> request;
};

class GenericResponse : public Response {
public:
    GenericResponse(Context c, Id id);

    QJsonDocument getContents() const override;

private:
    QJsonDocument contents;
};

/**
 * Tracks active message ID's, so we can link them to requests and detect
 * duplicates
 *
 * Definitely not thread safe. Needs to be linked with another, so responses
 * can be matched with requests of the other, and vice versa
 */
template <typename T>
class IdTracker {
public:
    IdTracker() = default;

    /** Links the two stores, so that retrievals work on the other one */
    void linkWith(IdTracker<T> &other);

    /**
     * Inserts the request into the tracker, returning a pointer to the old message stored
     * under that ID if it exists.
     */
    option<T> insert(Id id, T msg);

    /**
     * Returns a pointer to the message stored under that ID if it exists, and
     * removes the message from the tracker.
     */
    option<T> retrieve(Id id);

private:
    QMap<QString, T> stringIds {};
    QMap<int, T> numberIds {};

    IdTracker<T> *other;
};


/**
 * Takes in Messages and validates them against the LSP spec,
 * annotating them with as much information as possible, such as
 * message type (notification, request, response), schema errors,
 * minimum / maximum LSP version of message, etc.
 *
 * A validator is stateful; it tracks the capabilities, request response IDs,
 * etc.
 */
class LspSchemaValidator : public QObject {
    Q_OBJECT

public:
    LspSchemaValidator(Entity sender, QObject* parent = nullptr);

    void linkWith(LspSchemaValidator &other);

signals:
    void emitLspMessage(std::shared_ptr<Message> message);

public slots:
    void onMessage(MessageBuilder::Message message);

private:
    void onMessageBatch(MessageBuilder::Message message, QJsonArray batch);

    void onMessageObject(MessageBuilder::Message message, QJsonObject contents);

    void validateJsonrpcMember(Context &c, const QJsonObject &contents);

    void validateNotification(QString method, option<QJsonDocument> params, SchemaJson& issues);

    void validateRequest(option<Id> id, QString method, option<QJsonDocument> params, SchemaJson &issues, std::shared_ptr<LspMessage> msg);

    void validateResponseSuccess(Id id, QJsonValue result, SchemaJson& issues, std::shared_ptr<LspMessage> msg);

    void validateResponseError(QJsonValue errorMethod, SchemaJson& rootIssues);

    std::shared_ptr<Notification> buildNotification(Context c, QString method);

    std::shared_ptr<Request> buildRequest(Context c, QString method, Id id);

    std::shared_ptr<Response> buildResponse(Context c, Id id);

    Entity sender;

    IdTracker<std::shared_ptr<Request>> idTracker {};

};

}

Q_DECLARE_METATYPE(std::shared_ptr<Lsp::LspMessage>);
Q_DECLARE_METATYPE(std::shared_ptr<Lsp::Message>);

#endif // LSPSCHEMAVALIDATOR_H
