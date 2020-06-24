#ifndef STDIOMITM_H
#define STDIOMITM_H

#include <QObject>
#include <QProcess>

#include "stdinthread.h"
#include "commlog.h"
#include "lspmessagebuilder.h"

class StdioMitm : public QObject
{
    Q_OBJECT
public:
    explicit StdioMitm(QObject *parent = nullptr);

    void setServer(QProcess *server);

    void setLog(CommLog *log);

    void startPollingStdin();

public slots:
    void onStdin(QByteArray *data);

    void onServerStdout();

    void onServerFinish(int exitCode, QProcess::ExitStatus exitStatus);

    void onLspMessage(LspMessage *msg);

private:
    QProcess *server;

    StdinThread *stdinThread;

    CommLog *log;

    LspMessageBuilder clientBuilder {};

    LspMessageBuilder serverBuilder {};
};

#endif // STDIOMITM_H
