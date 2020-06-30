#include "stdiomitm.h"

#include <QtCore>
#include <QString>
#include <QScrollBar>
#include <iostream>

StdioMitm::StdioMitm(QObject *parent) : QObject(parent)
{
    debouncer.setSingleShot(true);
    input = new StdinStream(this);

    QObject::connect(input, &InputStream::emitInput, this, &StdioMitm::onStdin);

    QObject::connect(&clientBuilder, &LspMessageBuilder::emitLspMessage, this, &StdioMitm::onLspMessage);
    QObject::connect(&serverBuilder, &LspMessageBuilder::emitLspMessage, this, &StdioMitm::onLspMessage);

    QObject::connect(&debouncer, &QTimer::timeout, this, &StdioMitm::onDebounceEnd);
}

void StdioMitm::onDebounceEnd() {
    debouncing = false;
    if (!buffer.isEmpty()) {
        messages.append(buffer);
        buffer.clear();
    }
}

void StdioMitm::startPollingStdin() {
    input->start();
}

void StdioMitm::setServer(QProcess *server) {
    this->server = server;
}

void StdioMitm::onStdin(QByteArray data) {
    qDebug() << "got data";

    server->write(data);
    clientBuilder.append(data);
}

void StdioMitm::onServerStdout() {
    QByteArray buff = server->readAllStandardOutput();

    serverBuilder.append(buff);

    std::cout.write(buff, buff.size()).flush();
}

void StdioMitm::onServerStderr() {
    QByteArray buff = server->readAllStandardError();

    std::cerr << buff.toStdString() << std::endl;
}

void StdioMitm::onLspMessage(LspMessage *msg) {
    qDebug() << "received " << lspEntityToQString(msg->sender) << " msg";

    if (debouncing) {
        buffer.append(msg);
    } else {
        QVector<LspMessage*> m { msg } ;
        messages.append(m);

        debouncing = true;
        debouncer.start(100);
    }
}

void StdioMitm::onServerFinish(int exitCode, QProcess::ExitStatus exitStatus) {
    std::cerr << "Server closed with code " << exitCode << ", status " << exitStatus << std::endl;
}
