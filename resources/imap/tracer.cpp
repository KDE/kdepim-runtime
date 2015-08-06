#include "tracer.h"
#include "imapresource_debug.h"

#include <QString>
#include <QIODevice>
#include <QCoreApplication>
#include <iostream>
#include <unistd.h>

class DebugStream: public QIODevice
{
public:
    QString m_location;
    DebugStream()
        : QIODevice()
    {
        open(WriteOnly);
    }
    virtual ~DebugStream() {};

    bool isSequential() const Q_DECL_OVERRIDE
    {
        return true;
    }
    qint64 readData(char *, qint64) Q_DECL_OVERRIDE {
        return 0; /* eof */
    }
    qint64 readLineData(char *, qint64) Q_DECL_OVERRIDE {
        return 0; /* eof */
    }
    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE {
        const QByteArray buf = QByteArray::fromRawData(data, len);
        if (!qgetenv("IMAP_TRACE").isEmpty())
        {
            // qt_message_output(QtDebugMsg, buf.trimmed().constData());
            std::cout << buf.trimmed().constData() << std::endl;
        }
        return len;
    }
private:
    Q_DISABLE_COPY(DebugStream)
};

QDebug debugStream(int line, const char *file, const char *function)
{
    static DebugStream stream;
    QDebug debug(&stream);

    static QByteArray programName;
    if (programName.isEmpty()) {
        if (QCoreApplication::instance()) {
            programName = QCoreApplication::instance()->applicationName().toLocal8Bit();
        } else {
            programName = "<unknown program name>";
        }
    }

    debug << QString("Trace:%1(%2) %3:").arg(QString::fromLatin1(programName)).arg(unsigned(getpid())).arg(function) /* << file << ":" << line */;

    return debug;
}
