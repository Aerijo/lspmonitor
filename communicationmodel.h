#ifndef COMMUNICATIONMODEL_H
#define COMMUNICATIONMODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QIcon>
#include <QSortFilterProxyModel>

#include "lspschemavalidator.h"

class CommunicationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CommunicationModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void append(std::shared_ptr<Lsp::LspMessage> msg);

    void append(QVector<std::shared_ptr<Lsp::LspMessage>> msgs);

private slots:
    void onDebounceEnd();

private:
    QVector<std::shared_ptr<Lsp::LspMessage>> messages {};

    QVector<std::shared_ptr<Lsp::LspMessage>> debounceBuffer {};

    bool debouncing = false;

    int debouncems = 100;
};

class CommunicationDelegate : public QStyledItemDelegate {
public:
    CommunicationDelegate() : QStyledItemDelegate(nullptr) {
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

        std::shared_ptr<Lsp::LspMessage> msg = qvariant_cast<std::shared_ptr<Lsp::LspMessage>>(index.data());

        QString sum;

        QDateTime timestamp;
        timestamp.setTime_t(msg->timestamp / 1000);
        sum += timestamp.toString(Qt::SystemLocaleShortDate) + ": ";

        if (msg->kind == Lsp::LspMessage::Kind::Response && msg->match) {
            sum += "(+" + QString::number(msg->timestamp - msg->match->timestamp) + ") ";
        }

        switch (msg->sender) {
            case Lsp::LspMessage::Sender::Client:
                sum += "Client";
                break;
            case Lsp::LspMessage::Sender::Server:
                sum += "Server";
                break;
            default:
                sum += "Unknown";
                break;
        }

        sum += " sent a ";

        QIcon icon = unknown;

        switch (msg->kind) {
            case Lsp::LspMessage::Kind::Notification:
                sum += "Notification (" + msg->contents["method"].toString() + ")";
                icon = notifClient;
                break;
            case Lsp::LspMessage::Kind::Request:
                sum += "Request (" + QString::number(msg->contents["id"].toInt()) + ", " + msg->contents["method"].toString() + ")";
                icon = requestClient;
                break;
            case Lsp::LspMessage::Kind::Response:
                sum += "Response (" + QString::number(msg->contents["id"].toInt()) + ", " + (msg->match ? msg->match->contents["method"].toString() : "UNKNOWN") + ")";
                icon = responseClient;
                break;
            case Lsp::LspMessage::Kind::Batch:
                sum += "Batch";
                break;
            case Lsp::LspMessage::Kind::Unknown:
                sum += "UNKNOWN";
                break;
        }


        sum += " with " + QString::number(msg->issues.issueCount()) + " issues";
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
            painter->setPen(option.palette.text().color());
        }

        icon.paint(painter, iconRect, Qt::AlignCenter, selectMode);
        if (msg->issues.issueCount() > 0) {
            error.paint(painter, errorRect, Qt::AlignCenter, selectMode);
        }

//        QRect box (space);
//        box.adjust(1, 1, -1, -1);
//        painter->drawRect(box);

        painter->drawText(space, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QString text = qvariant_cast<QString>(index.data());
        QFontMetrics met (QFont ("Source Code Pro"));
        return met.size(0, text);
    }
};

struct LogFilter {
    bool filterMethod = true;
    bool exactMethodMatch = false;
    QString method = "";

    bool filterSender = false;
    Lsp::LspMessage::Sender sender = Lsp::LspMessage::Sender::Client;

    bool filterKind = false;
    QVector<Lsp::LspMessage::Kind> kinds {};

    bool check(const Lsp::LspMessage &msg) const {
        if (filterMethod) {
            if (method.size() > msg.method.size()) {
                return false;
            }

            if (exactMethodMatch) {
                if (!msg.method.startsWith(method)) {
                    return false;
                }
            } else {
                int i = 0;

                for (QChar c : method) {
                    bool found = false;
                    for (; i < msg.method.size(); i++) {
                        if (msg.method.at(i) == c) {
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
            if (msg.sender != sender) {
                return false;
            }
        }

        if (filterKind) {
            if (!kinds.contains(msg.kind)) {
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
        auto msg = qvariant_cast<std::shared_ptr<Lsp::LspMessage>>(v);
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

#endif // COMMUNICATIONMODEL_H
