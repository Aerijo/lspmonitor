#ifndef COMMLOG_H
#define COMMLOG_H

#include <QPlainTextEdit>


class CommLog : public QPlainTextEdit
{
    Q_OBJECT

public:
    CommLog(QWidget *parent = nullptr) : QPlainTextEdit(parent) {
        this->setReadOnly(true);
    }

};

#endif // COMMLOG_H
