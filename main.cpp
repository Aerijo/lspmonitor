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
#include <QPushButton>

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

    auto window = new QMainWindow();

    auto mainWidget = new QWidget (window);
    window->setCentralWidget(mainWidget);

    auto mainLayout = new QVBoxLayout (mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto logSplitter = new QSplitter(mainWidget);
    mainLayout->addWidget(logSplitter);

    logSplitter->setOrientation(Qt::Horizontal);

    auto historyWidget = new QWidget(logSplitter);
    logSplitter->addWidget(historyWidget);

    auto historyLayout = new QVBoxLayout(historyWidget);

    auto inputs = new QHBoxLayout(mainWidget);
    historyLayout->addItem(inputs);

    auto filterInput = new QLineEdit(historyWidget);
    inputs->addWidget(filterInput);

    QObject::connect(filterInput, &QLineEdit::textChanged, filtered, &FilteredCommModel::updateFilter);

    auto saveButton = new QPushButton(historyWidget);
    inputs->addWidget(saveButton);

    saveButton->setText("Save");
    saveButton->setContentsMargins(5, 0, 5, 0);
    QObject::connect(saveButton, &QPushButton::clicked, &mitm->messages, &CommunicationModel::save);

    auto logView = new QListView(historyWidget);
    historyLayout->addWidget(logView);

    auto del = new CommunicationDelegate(logView);

    logView->setModel(filtered);
    logView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    logView->verticalScrollBar()->setSingleStep(25);
    logView->setItemDelegate(del);
    logView->setAutoScroll(false);
    logView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    bool *logAtBottom = new bool();
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsAboutToBeInserted, [=]{ *logAtBottom = logView->verticalScrollBar()->maximum() == logView->verticalScrollBar()->value(); });
    QObject::connect(&mitm->messages, &QAbstractListModel::rowsInserted, [=]{ if (*logAtBottom) { logView->scrollToBottom(); } });

    logView->setMouseTracking(true);
    QObject::connect(logView, &QListView::entered, &mitm->messages, &CommunicationModel::entered);

    auto detailView = new DetailedViewWidget(logSplitter);
    logSplitter->addWidget(detailView);

    QObject::connect(logView->selectionModel(), &QItemSelectionModel::currentChanged, [=](const QModelIndex& current, const QModelIndex &){ detailView->onMessageChange(qvariant_cast<LspMessageItem>(current.data()).message); });

    mainWidget->setLayout(mainLayout);
    window->show();

    return app->exec();
}
