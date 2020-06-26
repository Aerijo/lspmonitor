#include "stdiomitm.h"

#include <QtCore>
#include <QString>
#include <QScrollBar>
#include <iostream>

StdioMitm::StdioMitm(QObject *parent) : QObject(parent)
{
    stdinThread = new StdinThread(this);
    QObject::connect(stdinThread, &StdinThread::onStdin, this, &StdioMitm::onStdin);

    QObject::connect(&clientBuilder, &LspMessageBuilder::emitLspMessage, this, &StdioMitm::onLspMessage);
    QObject::connect(&serverBuilder, &LspMessageBuilder::emitLspMessage, this, &StdioMitm::onLspMessage);
}

void StdioMitm::startPollingStdin() {
    stdinThread->start();
}

void StdioMitm::setServer(QProcess *server) {
    this->server = server;
}

void StdioMitm::setLog(CommLog *log) {
    this->log = log;
}

void StdioMitm::onStdin(QByteArray *data) {
    server->write(*data);

    clientBuilder.append(data);

    delete data;
}

void StdioMitm::onServerStdout() {
    QByteArray buff = server->readAllStandardOutput();

    serverBuilder.append(&buff);

    std::cout.write(buff, buff.size()).flush();
}

void StdioMitm::onServerStderr() {
    QByteArray buff = server->readAllStandardError();

    std::cerr << buff.toStdString() << std::endl;
}

void StdioMitm::onLspMessage(LspMessage *msg) {
    if (log != nullptr) {
        log->moveCursor(QTextCursor::End);
        log->insertPlainText(lspEntityToQString(msg->sender) + " (" + QString::number(msg->timestamp) + ") -> " + msg->message.toJson());
        log->verticalScrollBar()->setValue(log->verticalScrollBar()->maximum());
    }

    messages.append(msg);
}

void StdioMitm::onServerFinish(int exitCode, QProcess::ExitStatus exitStatus) {
    std::cerr << "Server closed with code " << exitCode << ", status " << exitStatus << std::endl;

    if (log != nullptr) {
        log->moveCursor(QTextCursor::End);
        log->insertPlainText("Server closed with code " + QString::number(exitCode) + ", status " + QString::number(exitStatus));
        log->verticalScrollBar()->setValue(log->verticalScrollBar()->maximum());
    }

//    qApp->exit(exitCode);
}
