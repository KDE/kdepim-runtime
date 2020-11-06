/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KCompositeJob>

#include <KIMAP/ListJob>

namespace KIMAP {
class Session;
}

class PrepareSessionJob : public KCompositeJob
{
    Q_OBJECT
public:
    enum Error {
        CapabilitiesTestError = KJob::UserDefinedError,
        NamespaceFetchError,
        IdentificationError
    };

    explicit PrepareSessionJob(KIMAP::Session *session, const QByteArray &clientId);

    void start() override;

    KIMAP::Session *session() const { return m_session; }
    QStringList capabilities() const { return m_capabilities; }
    QList<KIMAP::MailBoxDescriptor> namespaces() const { return m_namespaces; }
    QList<KIMAP::MailBoxDescriptor> personalNamespaces() const { return m_personalNamespaces; }
    QList<KIMAP::MailBoxDescriptor> userNamespaces() const { return m_userNamespaces; }
    QList<KIMAP::MailBoxDescriptor> sharedNamespaces() const { return m_sharedNamespaces; }

protected:
    void slotResult(KJob *job) override;

private:
    bool handleError(KJob *job, Error error);

    void capabilitiesJobDone(KJob *job);
    void namespaceJobDone(KJob *job);
    void idJobDone(KJob *job);

private:
    KIMAP::Session * const m_session;
    const QByteArray m_clientId;

    QStringList m_capabilities;

    QList<KIMAP::MailBoxDescriptor> m_namespaces;
    QList<KIMAP::MailBoxDescriptor> m_personalNamespaces;
    QList<KIMAP::MailBoxDescriptor> m_userNamespaces;
    QList<KIMAP::MailBoxDescriptor> m_sharedNamespaces;
};
