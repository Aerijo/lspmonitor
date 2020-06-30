#include "msglogmodel.h"

MsgLogModel::MsgLogModel(QObject *parent)
    : QAbstractListModel(parent), messages(QVector<LspMessage*> ())
{
}

int MsgLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return messages.size();
}

QVariant MsgLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qDebug() << "Not valid";
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        QVariant s;
        LspMessage *msg = messages[index.row()];
        s.setValue(*msg);
        return s;
    }

    return QVariant();
}


void MsgLogModel::append(QVector<LspMessage*> &msgs) {
    // TODO: Debounce updates
    beginInsertRows(QModelIndex(), messages.size(), messages.size() + msgs.size() - 1);
    for (LspMessage* msg : msgs) {
        messages.append(msg);
    }
    endInsertRows();
}
