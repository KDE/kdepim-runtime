#include "jobs.h"

Job::Job( QObject* parent )
    : KJob( parent )
{
}

void Job::start()
{
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}

void Job::setUrl( const KUrl& url )
{
    m_url = url;
}

void Job::setPassword( const QString& password )
{
    m_password = password;
}

KUrl Job::url() const
{
    return m_url;
}

QString Job::password() const
{
    return m_password;
}

ListSubscriptionsJob::ListSubscriptionsJob( QObject* parent )
    : Job( parent )
{
}

void ListSubscriptionsJob::doStart()
{
}

#include "jobs.moc"
