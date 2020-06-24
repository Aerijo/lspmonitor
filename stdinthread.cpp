#include "stdinthread.h"

#include <QByteArray>

StdinThread::StdinThread(QObject *parent) : QThread(parent) {
}

void StdinThread::run() {
    while (true) {
        char c = std::cin.get();
        QByteArray *buffer = new QByteArray {};
        buffer->append(c);
        emit onStdin(buffer);
    }
}
