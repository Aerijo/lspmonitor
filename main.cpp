#include <QApplication>
#include <QCommandLineParser>
#include <QSocketNotifier>
#include <iostream>
#include <unistd.h>
#include <string>
#include <QProcess>
#include <QThread>

#include "stdinthread.h"
#include "stdiomitm.h"
#include "commlog.h"

QCoreApplication* createApplication(int &argc, char *argv[])
{
    return new QApplication(argc, argv);


//    for (int i = 1; i < argc; ++i) {
//        if (!qstrcmp(argv[i], "--gui")) {
//            return new QApplication(argc, argv);
//        }
//    }

//    return new QCoreApplication(argc, argv);
}

int main(int argc, char** argv) {
    QScopedPointer<QCoreApplication> app(createApplication(argc, argv));
    QCoreApplication::setApplicationName("lspmonitor");
    QCoreApplication::setApplicationVersion("0.0.0");

    QCommandLineParser parser {};
    parser.setApplicationDescription("Tools to monitor and interact with LSP servers and clients");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption guiOpt ( "gui", "Launch an untied GUI instance to monitor the communications" );
    parser.addOption(guiOpt);

    parser.process(*app->instance());

    QStringList args = parser.positionalArguments();
    if (args.size() < 1) {
        std::cerr << "No server path given, exiting" << std::endl;
        return -1;
    }

    CommLog *log = new CommLog(nullptr);
    log->show();

    QString target = args[0];
    std::cerr << "opening target: " << target.toStdString() << std::endl;

    QProcess *serverProcess = new QProcess(app.data());
    serverProcess->setProgram(target);
    serverProcess->setArguments(args.mid(1, args.size() - 2));
    serverProcess->start();

    std::cerr << "started target" << std::endl;

    StdioMitm *mitm = new StdioMitm(nullptr);
    mitm->setServer(serverProcess);
    mitm->setLog(log);

    QObject::connect(serverProcess, &QProcess::readyReadStandardOutput, mitm, &StdioMitm::onServerStdout); // server stdout -> stdout
    QObject::connect(serverProcess, &QProcess::readyReadStandardError, mitm, &StdioMitm::onServerStderr);
    QObject::connect(serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), mitm, &StdioMitm::onServerFinish);

    mitm->startPollingStdin();

    return app->exec();
}
