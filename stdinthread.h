#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include <iostream>
#include <QThread>
#include <QProcess>

class StdinThread : public QThread
{
    Q_OBJECT

    QProcess* server;

public:
    StdinThread(QObject *parent);

    void run() override;

signals:
    void onStdin(QByteArray *data);
};

#endif // CLIENTTHREAD_H
