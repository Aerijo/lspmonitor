#include <QApplication>
#include <QCommandLineParser>
#include <QSocketNotifier>
#include <iostream>
#include <unistd.h>
#include <string>
#include <QProcess>
#include <QThread>
#include <QStandardItemModel>
#include <QListView>
#include <QItemSelectionModel>

#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QScrollBar>

#include "connectionstream.h"
#include "stdiomitm.h"
#include "msglogmodel.h"

QCoreApplication* createApplication(int &argc, char *argv[])
{
    return new QApplication(argc, argv);
}

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);

    QApplication* app = new QApplication(argc, argv);
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

    QString target = args[0];
    std::cerr << "opening target: " << target.toStdString() << std::endl;

    QProcess *serverProcess = new QProcess(app);
    serverProcess->setProgram(target);
    serverProcess->setArguments(args.mid(1, args.size() - 2));
    serverProcess->start();

    std::cerr << "started target" << std::endl;

    StdioMitm *mitm = new StdioMitm(nullptr);
    mitm->setServer(serverProcess);

    QObject::connect(serverProcess, &QProcess::readyReadStandardOutput, mitm, &StdioMitm::onServerStdout); // server stdout -> stdout
    QObject::connect(serverProcess, &QProcess::readyReadStandardError, mitm, &StdioMitm::onServerStderr);
    QObject::connect(serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), mitm, &StdioMitm::onServerFinish);

    qDebug() << "Hah";

    mitm->startPollingStdin();

    qDebug() << "Hmm";

    QListView *view = new QListView;
    view->setModel(&mitm->messages);
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->verticalScrollBar()->setSingleStep(25);

    MsgLogDelegate *del = new MsgLogDelegate();
    view->setItemDelegate(del);
    view->setAutoScroll(false);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->show();


    return app->exec();
}
