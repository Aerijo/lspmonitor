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

#include "communicationmodel.h"
#include "connectionstream.h"
#include "stdiomitm.h"

int main(int argc, char** argv) {
    // Currently causing bugs when enabled
    // std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

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

    StdioMitm *mitm = new StdioMitm(serverProcess, nullptr);

    mitm->start();

    QListView *view = new QListView;
    view->setModel(&mitm->messages);
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->verticalScrollBar()->setSingleStep(25);

    CommunicationDelegate *del = new CommunicationDelegate();
    view->setItemDelegate(del);
    view->setAutoScroll(false);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->show();

    bool *atBottom = new bool();
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsAboutToBeInserted, [=]{ *atBottom = view->verticalScrollBar()->maximum() == view->verticalScrollBar()->value(); });
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsInserted, view, [=]{ if (*atBottom) { view->scrollToBottom(); } });

    return app->exec();
}
