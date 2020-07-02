#include "communicationmodel.h"

CommunicationModel::CommunicationModel(QObject* parent) : QAbstractListModel(parent) {

}

int CommunicationModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }

    return messages.size();
}

QVariant CommunicationModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        QVariant s;
        Lsp::LspMessage const &msg = messages[index.row()];
        s.setValue(msg);
        return s;
    } else {
        return QVariant();
    }
}

void CommunicationModel::append(Lsp::LspMessage msg) {
    append(QVector<Lsp::LspMessage> { msg });
}

void CommunicationModel::append(QVector<Lsp::LspMessage> msgs) {
    if (debouncing) {
        debounceBuffer.append(msgs);
        return;
    }

    debouncing = true;
    QTimer::singleShot(debouncems, this, &CommunicationModel::onDebounceEnd);

    beginInsertRows(QModelIndex(), messages.size(), messages.size() + msgs.size() - 1);
    messages.append(msgs);
    endInsertRows();
}

void CommunicationModel::onDebounceEnd() {
    if (debounceBuffer.isEmpty()) {
        debouncing = false;
        return;
    }

    beginInsertRows(QModelIndex(), messages.size(), messages.size() + debounceBuffer.size() - 1);
    messages.append(debounceBuffer);
    endInsertRows();

    debounceBuffer.clear();
    QTimer::singleShot(debouncems, this, &CommunicationModel::onDebounceEnd);
}
