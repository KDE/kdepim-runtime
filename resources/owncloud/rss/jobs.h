#ifndef OWNCLOUD_JOBS_H
#define OWNCLOUD_JOBS_H

#include <KJob>
#include <KUrl>

class Job : public KJob {
    Q_OBJECT
public:
    explicit Job( QObject* parent );

    void start();

    KUrl url() const;
    void setUrl( const KUrl& );

    QString password() const;
    void setPassword( const QString& password );

protected:
    Q_INVOKABLE virtual void doStart() = 0;
private:
    KUrl m_url;
    QString m_password;
};

class ListSubscriptionsJob : public Job {
    Q_OBJECT
public:
    explicit ListSubscriptionsJob( QObject* parent );

protected:
    void doStart();
};

#endif
