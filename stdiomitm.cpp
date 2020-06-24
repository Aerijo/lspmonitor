#include "stdiomitm.h"

#include <QtCore>
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

void StdioMitm::onLspMessage(LspMessage *msg) {
    if (log != nullptr) {
        log->insertPlainText(msg->message.toJson());
        log->verticalScrollBar()->setValue(log->verticalScrollBar()->maximum());
    }

    delete msg;
}

void StdioMitm::onServerFinish(int exitCode, QProcess::ExitStatus exitStatus) {
    std::cerr << "Server closed with code " << exitCode << ", status " << exitStatus << std::endl;
    qApp->exit(exitCode);
}
