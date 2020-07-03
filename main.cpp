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
#include <QMainWindow>
#include <QSplitter>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWindow>

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

    FilteredCommModel *filtered = new FilteredCommModel();
    filtered->setSourceModel(&mitm->messages);

    QMainWindow *window = new QMainWindow();
    QSplitter *splitter = new QSplitter(window);
    splitter->setOrientation(Qt::Vertical);
    window->setCentralWidget(splitter);


    QListView *view = new QListView(splitter);
    view->setModel(&mitm->messages);
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->verticalScrollBar()->setSingleStep(25);

    splitter->addWidget(view);



    QLineEdit *filterInput = new QLineEdit();

    QObject::connect(filterInput, &QLineEdit::textChanged, filtered, &FilteredCommModel::updateFilter);

    QListView *fview = new QListView(splitter);
    fview->setModel(filtered);
    fview->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    fview->verticalScrollBar()->setSingleStep(25);



    QVBoxLayout *flay = new QVBoxLayout(splitter);
    flay->setContentsMargins(0, 0, 0, 0);
    flay->addWidget(filterInput);
    flay->addWidget(fview);

    QWidget *fwin = new QWidget(splitter);
    fwin->setLayout(flay);

    splitter->addWidget(fwin);

    CommunicationDelegate *del = new CommunicationDelegate();
    view->setItemDelegate(del);
    view->setAutoScroll(false);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    fview->setItemDelegate(del);
    fview->setAutoScroll(false);
    fview->setEditTriggers(QAbstractItemView::NoEditTriggers);

    bool *atBottom = new bool();
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsAboutToBeInserted, [=]{ *atBottom = view->verticalScrollBar()->maximum() == view->verticalScrollBar()->value(); });
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsInserted, view, [=]{ if (*atBottom) { view->scrollToBottom(); } });

    bool *atBottomf = new bool();
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsAboutToBeInserted, [=]{ *atBottomf = fview->verticalScrollBar()->maximum() == fview->verticalScrollBar()->value(); });
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsInserted, fview, [=]{ if (*atBottomf) { fview->scrollToBottom(); } });


    window->show();

    return app->exec();
}
