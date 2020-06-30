#ifndef STDIOMITM_H
#define STDIOMITM_H

#include <QObject>
#include <QProcess>
#include <QTimer>

#include "connectionstream.h"
#include "lspmessagebuilder.h"
#include "msglogmodel.h"

class StdioMitm : public QObject
{
    Q_OBJECT
public:
    explicit StdioMitm(QObject *parent = nullptr);

    void setServer(QProcess *server);

    void startPollingStdin();

    MsgLogModel messages;


public slots:
    void onStdin(QByteArray data);

    void onServerStdout();

    void onServerStderr();

    void onServerFinish(int exitCode, QProcess::ExitStatus exitStatus);

    void onLspMessage(LspMessage *msg);

    void onDebounceEnd();


private:
    QProcess *server;

    InputStream *input;

    LspMessageBuilder clientBuilder { LspEntity::Client };

    LspMessageBuilder serverBuilder { LspEntity::Server };

    QVector<LspMessage*> buffer {};

    QTimer debouncer {};

    bool debouncing = false;
};

#endif // STDIOMITM_H
