#include "communicationmodel.h"
#include <QFileDialog>
#include <QMessageBox>

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
        std::shared_ptr<Lsp::Message> msg = messages[index.row()];

        LspMessageItem item {};
        item.message = msg;


        s.setValue(item);
        return s;
    } else {
        return QVariant();
    }
}

void CommunicationModel::append(std::shared_ptr<Lsp::Message> msg) {
    append(QVector<std::shared_ptr<Lsp::Message>> { msg });
}

void CommunicationModel::append(QVector<std::shared_ptr<Lsp::Message>> msgs) {
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

void CommunicationModel::save() {
    QString filePath = QFileDialog::getSaveFileName(nullptr, tr("Save log"));

    if (filePath.isEmpty()) {
        return;
    }

    QFile file (filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::information(nullptr, tr("Unable to open file"), file.errorString());
        return;
    }

    QTextStream out (&file);
    out << this->serialize();
}

QByteArray CommunicationModel::serialize() {
    QByteArray data;

    for (auto msg : messages) {
        if (msg->getSender() == Lsp::Entity::Client) {
            data.append("<--");
        } else {
            data.append("-->");
        }

        data.append(" " + QString::number(msg->getTimestamp()));

        data.append(" " + msg->getContents().toJson(QJsonDocument::Compact));

        data.append("\n");
    }

    return data;
}

void CommunicationModel::deserialize(QByteArray data) {
    // TODO
}

void CommunicationModel::entered(const QModelIndex &index) {
    if (index.row() < 0 || index.row() >= messages.size()) {
        return;
    }

//    auto oldActive = active;
//    auto oldActivePair = activePair;

//    active = index.row();

//    if (oldActive != active) {
////        activePair = messages[active]->

//        emit dataChanged(this->index(oldActive), this->index(oldActive));
//        emit dataChanged(this->index(active), this->index(active));

//        if (messages[oldActive]->match) {
//            emit dataChanged(this->index(oldActive), this->index(oldActive));
//        }
//    }
}
