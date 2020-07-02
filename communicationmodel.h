#ifndef COMMUNICATIONMODEL_H
#define COMMUNICATIONMODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QIcon>

#include "lspschemavalidator.h"

class CommunicationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CommunicationModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void append(Lsp::LspMessage msg);

    void append(QVector<Lsp::LspMessage> msgs);

private slots:
    void onDebounceEnd();

private:
    QVector<Lsp::LspMessage> messages {};

    QVector<Lsp::LspMessage> debounceBuffer {};

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

        Lsp::LspMessage msg = qvariant_cast<Lsp::LspMessage>(index.data());



        QString sum = "(" + QString::number(msg.timestamp) + ") ";

        switch (msg.sender) {
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

        switch (msg.kind) {
            case Lsp::LspMessage::Kind::Notification:
                sum += "Notification (" + msg.contents["method"].toString() + ")";
                icon = notifClient;
                break;
            case Lsp::LspMessage::Kind::Request:
                sum += "Request (" + msg.contents["method"].toString() + ")";
                icon = requestClient;
                break;
            case Lsp::LspMessage::Kind::Response:
                sum += "Response";
                icon = responseClient;
                break;
            case Lsp::LspMessage::Kind::Batch:
                sum += "Batch";
                break;
            case Lsp::LspMessage::Kind::Unknown:
                sum += "UNKNOWN";
                break;
        }


        sum += " with " + QString::number(msg.issues.issueCount()) + " issues";
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
        if (msg.issues.issueCount() > 0) {
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


#endif // COMMUNICATIONMODEL_H
