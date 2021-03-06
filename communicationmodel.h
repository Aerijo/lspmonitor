#ifndef COMMUNICATIONMODEL_H
#define COMMUNICATIONMODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QIcon>
#include <QSortFilterProxyModel>

#include "lspschemavalidator.h"

struct LspMessageItem {
public:
    LspMessageItem() = default;

    std::shared_ptr<Lsp::Message> message;

    bool active = false;

    bool pair = false;
};

Q_DECLARE_METATYPE(LspMessageItem);

class CommunicationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CommunicationModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void append(std::shared_ptr<Lsp::Message> msg);

    void append(QVector<std::shared_ptr<Lsp::Message>> msgs);

    void save();

    void entered(const QModelIndex &index);

private slots:
    void onDebounceEnd();

private:
    QVector<std::shared_ptr<Lsp::Message>> messages {};

    QVector<std::shared_ptr<Lsp::Message>> debounceBuffer {};

    bool debouncing = false;

    int debouncems = 100;

    QByteArray serialize();

    void deserialize(QByteArray data);

    int active = -1; // The currently moused over message index

    int activePair = -1; // The pair to the moused over message index (Request <-> Response) (-1 if active is Notification)
};

class CommunicationDelegate : public QStyledItemDelegate {
public:
    CommunicationDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {
        notifClient = QIcon(":/icons/resources/bell-solid.svg");
        notifServer = notifClient;

        requestClient = QIcon(":/icons/resources/comment.svg");
        requestServer = requestClient;

        responseClient = QIcon(":/icons/resources/reply-solid.svg");
        responseServer = responseClient;

        unknown = QIcon(":/icons/resources/question-circle-solid.svg");
        error = QIcon(":/icons/resources/exclamation-circle.svg");
    }

    QIcon notifClient;
    QIcon notifServer;

    QIcon requestClient;
    QIcon requestServer;

    QIcon responseClient;
    QIcon responseServer;

    QIcon unknown;
    QIcon error;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->setRenderHints(QPainter::Antialiasing);

        auto item = qvariant_cast<LspMessageItem>(index.data());
        auto msg = item.message;

        QString sum;

        sum += QString::number(msg->getIndex()) + " ";

        QDateTime timestamp;
        timestamp.setTime_t(msg->getTimestamp() / 1000);
        sum += timestamp.toString(Qt::SystemLocaleShortDate) + ": ";

        if (msg->getKind() == Lsp::Message::Kind::Response) {
            auto response = static_cast<Lsp::Response*>(msg.get());
            auto duration = response->getDuration();
            sum += "(+" + QString::number(duration) + ") ";
        }

        switch (msg->getSender()) {
            case Lsp::Entity::Client:
                sum += "Client";
                break;
            case Lsp::Entity::Server:
                sum += "Server";
                break;
            default:
                sum += "UNKNOWN SENDER";
                break;
        }

        sum += " sent a ";

        QIcon icon = unknown;
        QString method = msg->tryGetMethod().value_or("UNKNOWN METHOD");

        switch (msg->getKind()) {
            case Lsp::Message::Kind::Notification:
                sum += "Notification (" + method + ")";
                icon = notifClient;
                break;
            case Lsp::Message::Kind::Request:
                sum += "Request (" + QString::number(msg->getContents()["id"].toInt()) + ", " + method + ")";
                icon = requestClient;
                break;
            case Lsp::Message::Kind::Response:
                sum += "Response (" + QString::number(msg->getContents()["id"].toInt()) + ", " + method + ")";
                icon = responseClient;
                break;
            case Lsp::Message::Kind::Batch:
                sum += "Batch";
                break;
            case Lsp::Message::Kind::Unknown:
                sum += "UNKNOWN";
                break;
        }

        sum += " with " + QString::number(msg->getIssueCount()) + " issues";
        QString text = sum;

        QRect iconRect = option.rect;
        iconRect.setWidth(iconRect.height());

        QRect errorRect = iconRect;
        errorRect.moveLeft(iconRect.width());

        QRect space = option.rect;
        space.setLeft(space.left() + iconRect.width() * 2);

        iconRect.adjust(1, 1, -1, -1);
        errorRect.adjust(1, 1, -1, -1);

        painter->setFont(QFont("Source Code Pro"));

        QIcon::Mode selectMode = option.state & QStyle::State_Selected ? QIcon::Selected : QIcon::Normal;

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
            painter->setPen(option.palette.highlightedText().color());
        } else {
            if (item.pair) {
                painter->fillRect(option.rect, QColor("#E17666"));
            }
            painter->setPen(option.palette.text().color());
        }

        icon.paint(painter, iconRect, Qt::AlignCenter, selectMode);
        if (msg->getIssueCount() > 0) {
            error.paint(painter, errorRect, Qt::AlignCenter, selectMode);
        }

        painter->drawText(space, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        auto item = qvariant_cast<LspMessageItem>(index.data());
        QString text = "foo";

        QFontMetrics met (QFont ("Source Code Pro"));
        return met.size(0, text);
    }
};

struct LogFilter {
    bool filterMethod = true;
    bool exactMethodMatch = false;
    QString method = "";

    bool filterSender = false;
    Lsp::Entity sender = Lsp::Entity::Client;

    bool filterKind = false;
    QVector<Lsp::Message::Kind> kinds {};

    bool check(const Lsp::Message &msg) const {
        if (filterMethod) {
            auto tryMethod = msg.tryGetMethod();
            if (!tryMethod) {
                return false;
            }

            auto msgMethod = tryMethod.value();

            if (method.size() > msgMethod.size()) {
                return false;
            }

            if (exactMethodMatch) {
                if (!msgMethod.startsWith(method)) {
                    return false;
                }
            } else {
                int i = 0;

                for (QChar c : method) {
                    bool found = false;
                    for (; i < msgMethod.size(); i++) {
                        if (msgMethod.at(i) == c) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        return false;
                    }
                }
            }
        }

        if (filterSender) {
            if (msg.getSender() != sender) {
                return false;
            }
        }

        if (filterKind) {
            if (!kinds.contains(msg.getKind())) {
                return false;
            }
        }

        return true;
    }

    LogFilter() = default;
    LogFilter(QString method) : method(method) {}
};

class FilteredCommModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    FilteredCommModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setFilter(LogFilter filter) { this->filter = filter; }

    void updateFilter(const QString &text) {
        filter.method = text;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
        auto v = sourceModel()->data(sourceModel()->index(sourceRow, 0, sourceParent));
        auto item = qvariant_cast<LspMessageItem>(v);
        auto msg = item.message;
        return filter.check(*msg);
    }


    bool allowByMethod(QString method) const {
        if (!filter.filterMethod) {
            return true;
        }

        return method.startsWith(filter.method);
    }

private:
    LogFilter filter;
};

#include <QLayout>
#include <QLabel>
#include <QTextEdit>

class DetailedViewWidget : public QWidget {
    Q_OBJECT

public:
    DetailedViewWidget(QWidget *parent = nullptr) : QWidget(parent) {
        layout.addWidget(&methodLabel);

        layout.addWidget(&contents);

        setLayout(&layout);
    }

public slots:
    void onMessageChange(std::shared_ptr<Lsp::Message> msg) {
        methodLabel.setText(msg->tryGetMethod().value_or("NO METHOD") + " (" + QString::number(msg->getSize()) + "b)");

        contents.setText(msg->getContents().toJson(QJsonDocument::Indented));
    };

private:
    QVBoxLayout layout {};

    QLabel methodLabel {};

    QTextEdit contents {};

};

#endif // COMMUNICATIONMODEL_H
