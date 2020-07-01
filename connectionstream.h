#ifndef CONNECTIONSTREAM_H
#define CONNECTIONSTREAM_H

#include <QObject>
#include <QThread>
#include <QProcess>

/**
 * An abstraction over input streams (Stdin, Unix domain socket, TCP, etc.)
 * that emits chunks of the stream as they come.
 */
class InputStream : public QObject {
    Q_OBJECT

public:
    explicit InputStream(QObject *parent = nullptr);

    /**
     * Begin monitoring the stream (e.g., a Stdin stream may use this to spawn the monitoring thread).
     */
    virtual void start() = 0;

signals:
    void emitInput(QByteArray input);

    void emitError();

    void emitClose();
};

class StdinThread : public QThread {
    Q_OBJECT

public:
    void run() override;

signals:
    void emitInput(QByteArray input);
};

class StdinStream : public InputStream {
    Q_OBJECT

public:
    explicit StdinStream(QObject *parent = nullptr);

    void start() override;

private:
    StdinThread inputThread;
};

class ProcessStdinStream : public InputStream {
    Q_OBJECT

public:
    ProcessStdinStream(QProcess *process, QObject *parent = nullptr);

    void start();

public slots:
    void onProcessStdoutReady();

private:
    QProcess *process;
};

class OutputStream : public QObject {
    Q_OBJECT

public:
    explicit OutputStream(QObject *parent = nullptr);

    void operator<<(QByteArray output);

public slots:
    virtual void onOutput(QByteArray output) = 0;
};

class StdoutStream : public OutputStream {
    Q_OBJECT

public:
    StdoutStream(QObject *parent);

public slots:
    void onOutput(QByteArray output) override;

};

class ProcessStdoutStream : public OutputStream {
    Q_OBJECT

public:
    ProcessStdoutStream(QProcess *process, QObject *parent);

public slots:
    void onOutput(QByteArray output) override;

private:
    QProcess *process;
};

#endif // CONNECTIONSTREAM_H
