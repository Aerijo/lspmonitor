#ifndef LSPSCHEMAVALIDATOR_H
#define LSPSCHEMAVALIDATOR_H

#include <QJsonDocument>
#include <QJsonObject>

#ifdef __cpp_lib_optional
#include <optional>
template <typename T>
using option = std::optional<T>;
#else
#include <experimental/optional>
template <typename T>
using option = std::experimental::optional<T>;
#endif



namespace Lsp {

struct Version {
    int major;
    int minor;
    int patch;
};

class Uri {

};

/**
 * @brief URI that refers to a document. Encoded as a string,
 * but URI structure expected.
 */
class DocumentUri {

};

/**
 * @brief Represents an LSP Id (number or string)
 */
class Id {

};

enum class Source {
    Client,
    Server,
};

enum class MessageKind {
    Unknown,
    Notification,
    Request,
    Response,
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

struct Position {
    qint64 line;
    qint64 character;
};

struct Range {
    Position start;
    Position end;
};

struct Location {
    DocumentUri uri;
    Range range;
};

struct LocationLink {
    option<Range> originSelectionRange;
    DocumentUri targetUri;
    Range targetRange;
    Range targetSelectionRange;
};

struct DiagnosticRelatedInformation {
    Location location;
    QString message;
};

struct Diagnostic {
    Range range;
    option<DiagnosticSeverity> severity;
    option<Id> code;
    QString source;
    QString message;
    option<QVector<DiagnosticTag>> tags;
    option<QVector<DiagnosticRelatedInformation>> relatedInformation;
};

struct Command {
    QString title;
    QString command;
    option<QVector<QJsonValue>> arguments;
};

struct TextDocumentIdentifier {
    DocumentUri uri;
};

struct VersionedTextDocumentIdentifier : public TextDocumentIdentifier {
    option<qint64> version;
};

struct TextEdit {
    Range range;
    QString newText;
};

struct FileOperation {};

struct TextDocumentEdit : public FileOperation {
    VersionedTextDocumentIdentifier textDocument;
    QVector<TextEdit> edits;
};

struct CreateFileOptions {
    option<bool> overwrite;
    option<bool> ignoreIfExists;
};

struct CreateFile : public FileOperation {
    DocumentUri uri;
    option<CreateFileOptions> options;
};

struct RenameFileOptions {
    option<bool> overwrite;
    option<bool> ignoreIfExists;
};

struct RenameFile : public FileOperation {
    DocumentUri oldUri;
    DocumentUri newUri;
    option<RenameFileOptions> options;
};

struct DeleteFileOptions {
    option<bool> recursive;
    option<bool> ignoreIfNotExists;
};

struct DeleteFile : public FileOperation {
    DocumentUri uri;
    option<DeleteFileOptions> options;
};

struct WorkspaceEdit {
    option<QMap<DocumentUri, QVector<TextEdit>>> changes;
    option<QVector<FileOperation*>> documentChanges;
};

struct WorkspaceEditClientCapabilities {
    option<bool> documentChanges;
    option<QVector<ResourceOperationKind>> resourceOperations;
    option<FailureHandlingKind> failureHandling;
};

struct TextDocumentItem {
    DocumentUri uri;
    QString languageId;
    qint64 version;
    QString text;
};

struct TextDocumentPositionParams {
    TextDocumentIdentifier textDocument;
    Position position;
};

struct DocumentFilter {
    option<QString> language;
    option<QString> scheme;
    option<QString> pattern;
};

using DocumentSelector = QVector<DocumentFilter>;

struct StaticRegistrationOptions {
    option<QString> id;
};

struct TextDocumentRegistrationOptions {
    option<DocumentSelector> documentSelector;
};

struct MarkupContent {
    MarkupKind kind;
    QString value;
};

struct WorkDoneProgressBegin {
    QString title;
    option<bool> cancellable;
    option<QString> message;
    option<int> percentage;
};

struct WorkDoneProgressReport {
    option<bool> cancellable;
    option<QString> message;
    option<int> percentage;
};

struct WorkDoneProgressEnd {
    option<QString> message;
};

using ProgressToken = Id;

struct WorkDoneProgressParams {
    option<ProgressToken> workDoneToken;
};

struct WorkDoneProgressOptions {
    option<bool> workDoneProgress;
};

struct ClientInfo {
    QString name;
    option<QString> version;
};

struct ServerInfo {
    QString name;
    option<QString> version;
};

struct TextDocumentSyncClientCapabilities {
    // TODO
};

struct CompletionClientCapabilities {
    // TODO
};

struct HoverClientCapabilities {
    // TODO
};

struct SignatureHelpClientCapabilities {
    // TODO
};

struct DeclarationClientCapabilities {
    // TODO
};

struct DefinitionClientCapabilities {
    // TODO
};

struct TypeDefinitionClientCapabilities {
    // TODO
};

struct ImplementationClientCapabilities {
    // TODO
};

struct ReferenceClientCapabilities {
    // TODO
};

struct DocumentHighlightClientCapabilities {
    // TODO
};

struct DocumentSymbolClientCapabilities {
    // TODO
};

struct CodeActionClientCapabilities {
    // TODO
};

struct CodeLensClientCapabilities {
    // TODO
};

struct DocumentLinkClientCapabilities {
    // TODO
};

struct DocumentColorClientCapabilities {
    // TODO
};

struct DocumentFormattingClientCapabilities {
    // TODO
};

struct DocumentRangeFormattingClientCapabilities {
    // TODO
};

struct DocumentOnTypeFormattingClientCapabilities {
    // TODO
};

struct RenameClientCapabilities {
    // TODO
};

struct PublishDiagnosticsClientCapabilities {
    // TODO
};

struct FoldingRangeClientCapabilities {
    // TODO
};

struct SelectionRangeClientCapabilities {
    // TODO
};

struct TextDocumentClientCapabilities {
    option<TextDocumentSyncClientCapabilities> synchronization;
    option<CompletionClientCapabilities> completion;
    option<HoverClientCapabilities> hover;
    option<SignatureHelpClientCapabilities> signatureHelp;
    option<DeclarationClientCapabilities> declaration;
    option<DefinitionClientCapabilities> definition;
    option<TypeDefinitionClientCapabilities> typeDefinition;
    option<ImplementationClientCapabilities> implementation;
    option<ReferenceClientCapabilities> references;
    option<DocumentHighlightClientCapabilities> documentHighlight;
    option<DocumentSymbolClientCapabilities> documentSymbol;
    option<CodeActionClientCapabilities> codeAction;
    option<CodeLensClientCapabilities> codeLens;
    option<DocumentLinkClientCapabilities> documentLink;
    option<DocumentColorClientCapabilities> colorProvider;
    option<DocumentFormattingClientCapabilities> formatting;
    option<DocumentRangeFormattingClientCapabilities> rangeFormatting;
    option<DocumentOnTypeFormattingClientCapabilities> onTypeFormatting;
    option<RenameClientCapabilities> rename;
    option<PublishDiagnosticsClientCapabilities> publishDiagnostics;
    option<FoldingRangeClientCapabilities> foldingRange;
    option<SelectionRangeClientCapabilities> selectionRange;
};

struct DidChangeConfigurationClientCapabilities {
    // TODO
};

struct DidChangeWatchedFilesClientCapabilities {
    // TODO
};

struct WorkspaceSymbolClientCapabilities {
    // TODO
};

struct ExecuteCommandClientCapabilities {
    // TODO
};

struct WindowClientCapabilities {
    // TODO
};

struct WorkspaceClientCapabilities {
    option<bool> applyEdit;
    option<WorkspaceEditClientCapabilities> workspaceEdit;
    option<DidChangeConfigurationClientCapabilities> didChangeConfiguration;
    option<DidChangeWatchedFilesClientCapabilities> didChangeWatchedFiles;
    option<WorkspaceSymbolClientCapabilities> symbol;
    option<ExecuteCommandClientCapabilities> executeCommand;
    option<bool> workspaceFolders;
    option<bool> configuration;
};

struct ClientCapabilities {
    option<WorkspaceClientCapabilities> workspace;
    option<TextDocumentClientCapabilities> textDocument;
    option<WindowClientCapabilities> window;
    option<QJsonValue> experimental;
};

struct WorkspaceFolder {
    // TODO
};

struct InitializeParams : public WorkDoneProgressParams {
    option<qint64> processId;
    option<ClientInfo> clientInfo;
    option<option<QString>> rootPath; // missing=none, null=some(none), string=some(some(value))
    option<DocumentUri> rootUri;
    option<QJsonValue> initializationOptions;
    ClientCapabilities capabilities;
    option<Trace> trace;
    option<option<QVector<WorkspaceFolder>>> workspaceFolders;
};

struct ServerCapabilities {
    // TODO
};

struct InitializeResult {
    ServerCapabilities capabilities;
    option<ServerInfo> serverInfo;
};

class Request;

class Response;

class ParsedMessage;

class Message {
public:
    static ParsedMessage fromJson(QJsonDocument json);

    virtual QJsonDocument toJson() const;

    /** Minimum LSP version this message is compatible with */
    Version getMinimumVersion() const;

    /** Maximum of introduced versions of present properties. E.g., if a Diagnostic has `tags`, then it would be 3.15.0 */
    Version getMaximumFeatureVersion() const;

private:
    /** Time this message was received (upon full parse) */
    qint64 timestamp;
};

class UnknownMessage : public Message {
private:
    QJsonDocument raw;
};

class Notification : public Message {
public:
    QString getMethod() const;

private:
    /** If the notification can be safely ignored by clients/servers */
    bool optional;
};

class Request : public Message {
public:
    QString getMethod() const;

private:
    Id id;

    Response* response { nullptr };
};

class Response : public Message {
public:
    bool isSuccessful() const;

private:
    option<Id> id;

    Request* request { nullptr };
};

class ResponseError {
    int code;

    QString message;

    QJsonValue* data;
};

class GenericNotification : public Notification {
private:
    QString method;

    QJsonValue* params;
};

class CancelRequestNotification : public Notification {
    explicit CancelRequestNotification(Id id);
};

class ProgressNotification : public Notification {
private:
    Id token;

    QJsonValue value;
};


enum class SchemaIssueSeverity {
    Error = 1,
    Warning = 2,
};

struct SchemaIssue {
    SchemaIssueSeverity severity;
    QString message;

    static SchemaIssue error(QString msg);
    static SchemaIssue error(QJsonValue* location, QString msg);

    SchemaIssue(SchemaIssueSeverity severity, QString message);
    SchemaIssue(QJsonValue* location, SchemaIssueSeverity severity, QString message);
};

class ParsedMessage {
public:
    ParsedMessage();
    ParsedMessage(Message* message);
    ParsedMessage(Message* message, SchemaIssue issue);
    ParsedMessage(Message* message, QVector<SchemaIssue> issues);

private:
    Message* message;
    QVector<SchemaIssue> issues;
};

/**
 * JSON like tree structure that is a subset of the message, with
 * entries nesting to reach all schema issues
 */
class SchemaValue {
    QVector<SchemaIssue> localIssues;
};

class SchemaObject : public SchemaValue {
    QMap<QString, SchemaValue> properties;
};

class SchemaArray : public SchemaValue {
    QVector<SchemaValue> values;
};

struct MessageParseResult {
    MessageKind kind;


};

/**
 * Custom schame validator to detect nonconformance with the LSP
 * specification.
 *
 * There are varying kinds of nonconformance, including
 * - Invalid message kind: it is not a notification, request, or response
 * - Additional property: a property that is not part of the messages schema
 * - Missing property: a property that must be present
 * - Conflicting property: a property that is superceeded by another one
 * - Incorrect type: the property value type is incorrect
 */
//class LspSchemaParser
//{
//public:
//    LspSchemaParser();

//    parse();
//};

}

#endif // LSPSCHEMAVALIDATOR_H
