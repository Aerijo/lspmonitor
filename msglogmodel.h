#ifndef MSGLOGMODEL_H
#define MSGLOGMODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextDocument>


#include "lspmessagebuilder.h"


class MsgLogDelegate : public QStyledItemDelegate {
public:
    MsgLogDelegate() : QStyledItemDelegate(nullptr) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->setRenderHints(QPainter::Antialiasing);

        LspMessage msg = qvariant_cast<LspMessage>(index.data());

        bool isClient = msg.sender == LspEntity::Client;

        painter->setFont(QFont("Source Code Pro"));

        QString text = lspEntityToQString(msg.sender) + " -> " + msg.message.toJson();

        int width = painter->fontMetrics().size(0, text).width();

        QRect space (option.rect);

        if (isClient) {
            space.setRight(space.left() + width);
        } else {
            space.setLeft(space.right() - width);
        }

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(space, option.palette.highlight());
            painter->setPen(option.palette.highlightedText().color());
        } else {
            painter->setPen(option.palette.text().color());
        }



//        doc.drawContents(painter, space);

        QRect box (space);
        box.adjust(1, 1, -1, -1);
        painter->drawRect(box);

        painter->drawText(space, Qt::AlignLeft | Qt::AlignVCenter, text);


////        QStyledItemDelegate::paint(painter, option, index);


//        painter->save();
//        painter->setClipRect(option.rect);
//        LspMessage msg = qvariant_cast<LspMessage>(index.data());
//        QString text = lspEntityToQString(msg.sender);

//        QTextDocument doc;
//        doc.setHtml(text);

////        option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);

////        painter->translate(option.rect.left(), option.rect.top());
////        QRect clip(0, 0, option.rect.width(), option.rect.height());
////        doc.drawContents(painter, clip);

//        if (option.state & QStyle::State_Selected) {
//            painter->fillRect(option.rect, option.palette.highlight());
//        } else {
//            painter->fillRect(option.rect, text.startsWith("Server") ? option.palette.mid() : option.palette.dark());
//            painter->setPen(option.palette.text().color());
//            painter->drawText(option.rect.topLeft(), text);
//        }

//        painter->end();
//        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        LspMessage msg = qvariant_cast<LspMessage>(index.data());
        QString text = lspEntityToQString(msg.sender) + " -> " + msg.message.toJson();

        QFontMetrics met (QFont ("Source Code Pro"));
        return met.size(0, text);
    }
};


class MsgLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MsgLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void append(QVector<LspMessage*> &msgs);


private:
    QVector<LspMessage*> messages;
};

#endif // MSGLOGMODEL_H
