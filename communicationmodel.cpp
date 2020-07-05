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
        item.active = active == index.row();
        item.pair = activePair == index.row();
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
    int index = messages.size() + debounceBuffer.size();
    for (auto msg : msgs) {
        msg->setIndex(index);
        index += 1;
    }

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
    if (index.row() < 0 || index.row() >= messages.size() || index.row() == active) {
        return;
    }

    auto oldActive = active;
    auto oldActivePair = activePair;

    active = qvariant_cast<LspMessageItem>(index.data()).message->getIndex();
    activePair = -1;

    if (oldActive >= 0) {
        emit dataChanged(this->index(oldActive), this->index(oldActive));
    }

    if (oldActivePair >= 0) {
        emit dataChanged(this->index(oldActivePair), this->index(oldActivePair));
    }

    auto activeMsg = messages[active];

    if (activeMsg->getKind() == Lsp::Message::Kind::Notification) {
        return;
    }

    if (activeMsg->getKind() == Lsp::Message::Kind::Request) {
        auto request = static_cast<Lsp::Request*>(activeMsg.get());
        if (request->getResponse()) {
            activePair = request->getResponse()->getIndex();
        }
    } else if (activeMsg->getKind() == Lsp::Message::Kind::Response) {
        auto response = static_cast<Lsp::Response*>(activeMsg.get());
        if (response->getRequest()) {
            activePair = response->getRequest()->getIndex();
        }
    }

    emit dataChanged(this->index(active), this->index(active));

    if (activePair >= 0) {
        emit dataChanged(this->index(activePair), this->index(activePair));
    }

}
