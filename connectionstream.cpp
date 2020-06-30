#include <iostream>

#include <QtCore>

#include "connectionstream.h"

InputStream::InputStream(QObject *parent) : QObject(parent) {}

OutputStream::OutputStream(QObject *parent) : QObject(parent) {}

void OutputStream::operator<<(QByteArray output) {
    onOutput(output);
}

StdinStream::StdinStream(QObject *parent) : InputStream(parent) {}

void StdinStream::start() {
    connect(&inputThread, &StdinThread::emitInput, this, &StdinStream::emitInput);
    inputThread.start();
}

void StdinThread::run() {
    while (true) {
        char c = std::cin.get();
        QByteArray buffer {};
        buffer.append(c);
        emit emitInput(buffer);
    }
}
