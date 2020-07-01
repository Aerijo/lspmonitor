#include <iostream>

#include <QtCore>

#include "connectionstream.h"

InputStream::InputStream(QObject *parent) : QObject(parent) {}

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

ProcessStdinStream::ProcessStdinStream(QProcess *process, QObject *parent) : InputStream(parent), process(process) {}

void ProcessStdinStream::start() {
    connect(process, &QProcess::readyReadStandardOutput, this, &ProcessStdinStream::onProcessStdoutReady);
    onProcessStdoutReady();
}

void ProcessStdinStream::onProcessStdoutReady() {
    QByteArray readyOut = process->readAllStandardOutput();

    if (readyOut.length() > 0) {
        emit emitInput(readyOut);
    }

}

OutputStream::OutputStream(QObject *parent) : QObject(parent) {}

void OutputStream::operator<<(QByteArray output) {
    onOutput(output);
}

StdoutStream::StdoutStream(QObject *parent) : OutputStream(parent) {}

void StdoutStream::onOutput(QByteArray output) {
    std::cout.write(output, output.size()).flush();
}

ProcessStdoutStream::ProcessStdoutStream(QProcess *process, QObject *parent) : OutputStream(parent), process(process) {}

void ProcessStdoutStream::onOutput(QByteArray output) {
    qint64 written = process->write(output);
    if (written != output.size()) {
        // emit error
    }
}




