#include "stdiomitm.h"

#include <QtCore>
#include <QString>
#include <QScrollBar>
#include <iostream>

StdioMitm::StdioMitm(QProcess *server, QObject *parent) : QObject(parent), server(server), clientValidator(Lsp::Entity::Client), serverValidator(Lsp::Entity::Server) {
    clientIn = std::make_unique<StdinStream>(this);
    clientOut = std::make_unique<StdoutStream>(this);

    serverIn = std::make_unique<ProcessStdinStream>(server, this);
    serverOut = std::make_unique<ProcessStdoutStream>(server, this);

    connect(clientIn.get(), &InputStream::emitInput, &clientFrames, &FrameBuilder::FrameBuilder::onInput);
    connect(serverIn.get(), &InputStream::emitInput, &serverFrames, &FrameBuilder::FrameBuilder::onInput);

    connect(clientIn.get(), &InputStream::emitInput, serverOut.get(), &OutputStream::onOutput);
    connect(serverIn.get(), &InputStream::emitInput, clientOut.get(), &OutputStream::onOutput);

    connect(&clientFrames, &FrameBuilder::FrameBuilder::emitError, this, &StdioMitm::onClientFrameError);
    connect(&serverFrames, &FrameBuilder::FrameBuilder::emitError, this, &StdioMitm::onServerFrameError);

    connect(&clientFrames, &FrameBuilder::FrameBuilder::emitFrame, &clientMessages, &MessageBuilder::MessageBuilder::onFrame);
    connect(&serverFrames, &FrameBuilder::FrameBuilder::emitFrame, &serverMessages, &MessageBuilder::MessageBuilder::onFrame);

    connect(&clientMessages, &MessageBuilder::MessageBuilder::emitMessage, &clientValidator, &Lsp::LspSchemaValidator::onMessage);
    connect(&serverMessages, &MessageBuilder::MessageBuilder::emitMessage, &serverValidator, &Lsp::LspSchemaValidator::onMessage);

    connect(&clientValidator, &Lsp::LspSchemaValidator::emitLspMessage, this, &StdioMitm::onClientLspMessage);
    connect(&serverValidator, &Lsp::LspSchemaValidator::emitLspMessage, this, &StdioMitm::onServerLspMessage);

    connect(server, &QProcess::readyReadStandardError, this, &StdioMitm::onServerStderr);
    connect(server, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &StdioMitm::onServerFinish);

    clientValidator.linkWith(serverValidator);
}

void StdioMitm::start() {
    clientIn->start();
    serverIn->start();
}

void StdioMitm::onClientIn(QByteArray data) {
    serverOut->onOutput(data);
}

void StdioMitm::onServerIn(QByteArray buff) {
    clientOut->onOutput(buff);
}

void StdioMitm::onServerStderr() {
    QByteArray buff = server->readAllStandardError();
    std::cerr << buff.toStdString() << std::endl;
}

void StdioMitm::onClientFrame(FrameBuilder::Frame frame) {
    qDebug() << "got client frame";
}

void StdioMitm::onServerFrame(FrameBuilder::Frame frame) {
    qDebug() << "got server frame";
}

void StdioMitm::onClientFrameError(FrameBuilder::StreamError error) {
    qDebug() << "got client frame error: " + error.toQString();
}

void StdioMitm::onServerFrameError(FrameBuilder::StreamError error) {
    qDebug() << "got server frame error: " + error.toQString();
}

void StdioMitm::onClientMessage(MessageBuilder::Message message) {
    qDebug() << "got client message";
}

void StdioMitm::onServerMessage(MessageBuilder::Message message) {
    qDebug() << "got server message";
}

void StdioMitm::onClientLspMessage(std::shared_ptr<Lsp::Message> message) {
    messages.append(message);
}

void StdioMitm::onServerLspMessage(std::shared_ptr<Lsp::Message> message) {
    messages.append(message);
}

void StdioMitm::onServerFinish(int exitCode, QProcess::ExitStatus exitStatus) {
    std::cerr << "Server closed with code " << exitCode << ", status " << exitStatus << std::endl;
}
