#ifndef STDIOMITM_H
#define STDIOMITM_H

#include <QObject>
#include <QProcess>
#include <QTimer>

#include "connectionstream.h"
#include "framebuilder.h"
#include "messagebuilder.h"
#include "lspschemavalidator.h"
#include "communicationmodel.h"

class StdioMitm : public QObject
{
    Q_OBJECT
public:
    explicit StdioMitm(QProcess *server, QObject *parent = nullptr);

    void start();

    CommunicationModel messages {};


public slots:
    void onClientIn(QByteArray data);

    void onServerIn(QByteArray data);

    void onClientFrame(FrameBuilder::Frame frame);

    void onServerFrame(FrameBuilder::Frame frame);

    void onClientFrameError(FrameBuilder::StreamError error);

    void onServerFrameError(FrameBuilder::StreamError error);

    void onClientMessage(MessageBuilder::Message message);

    void onServerMessage(MessageBuilder::Message message);

    void onClientLspMessage(std::shared_ptr<Lsp::Message> message);

    void onServerLspMessage(std::shared_ptr<Lsp::Message> message);

    void onServerStderr();

    void onServerFinish(int exitCode, QProcess::ExitStatus exitStatus);


private:
    QProcess *server;

    std::unique_ptr<InputStream> clientIn;

    std::unique_ptr<OutputStream> clientOut;

    std::unique_ptr<InputStream> serverIn;

    std::unique_ptr<OutputStream> serverOut;

    FrameBuilder::FrameBuilder clientFrames;

    FrameBuilder::FrameBuilder serverFrames;

    MessageBuilder::MessageBuilder clientMessages;

    MessageBuilder::MessageBuilder serverMessages;

    Lsp::LspSchemaValidator clientValidator;

    Lsp::LspSchemaValidator serverValidator;
};

#endif // STDIOMITM_H
