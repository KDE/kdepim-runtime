/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef KOLABRETRIEVECOLLECTIONSTASK_H
#define KOLABRETRIEVECOLLECTIONSTASK_H

#include <resourcetask.h>

#include <AkonadiCore/Collection>

#include <kimap/listjob.h>
#include <QElapsedTimer>
class KolabRetrieveCollectionsTask : public ResourceTask
{
    Q_OBJECT

public:
    explicit KolabRetrieveCollectionsTask(const ResourceStateInterface::Ptr &resource, QObject *parent = nullptr);
    ~KolabRetrieveCollectionsTask() override;

private Q_SLOTS:
    void onMailBoxesReceived(const QList<KIMAP::MailBoxDescriptor> &descriptors, const QList< QList<QByteArray> > &flags);
    void onMailBoxesReceiveDone(KJob *job);
    void onFullMailBoxesReceived(const QList<KIMAP::MailBoxDescriptor> &descriptors, const QList<QList<QByteArray> > &flags);
    void onFullMailBoxesReceiveDone(KJob *job);
    void onMetadataRetrieved(KJob *job);

protected:
    void doStart(KIMAP::Session *session) override;

private:
    void checkDone();
    Akonadi::Collection getOrCreateParent(const QString &parentPath);
    void createCollection(const QString &mailbox, const QList<QByteArray> &flags, bool isSubscribed);
    void setAttributes(Akonadi::Collection &c, const QStringList &pathParts, const QString &path);
    void applyRights(const QHash<QString, KIMAP::Acl::Rights> &rights);
    void applyMetadata(const QHash<QString, QMap<QByteArray, QByteArray> > &metadata);

    int mJobs;
    QHash<QString, Akonadi::Collection> mMailCollections;
    QSet<QString> mSubscribedMailboxes;
    QSet<QByteArray> mRequestedMetadata;
    KIMAP::Session *mSession = nullptr;
    QElapsedTimer mTime;
    //For implicit sharing
    const QByteArray cContentMimeTypes;
    const QByteArray cAccessRights;
    const QByteArray cImapAcl;
    const QByteArray cCollectionAnnotations;
    const QSet<QByteArray> cDefaultKeepLocalChanges;
    const QStringList cDefaultMimeTypes;
    const QStringList cCollectionOnlyContentMimeTypes;
};

class RetrieveMetadataJob : public KJob
{
    Q_OBJECT
public:
    RetrieveMetadataJob(KIMAP::Session *session, const QStringList &mailboxes, const QStringList &serverCapabilities, const QSet<QByteArray> &requestedMetadata, const QString &separator, const QList <KIMAP::MailBoxDescriptor > &sharedNamespace, const QList <KIMAP::MailBoxDescriptor > &userNamespace,
                        QObject *parent = nullptr);
    void start() override;

    QHash<QString, QMap<QByteArray, QByteArray> > mMetadata;
    QHash<QString, KIMAP::Acl::Rights> mRights;

private:
    void checkDone();
    int mJobs;
    QSet<QByteArray> mRequestedMetadata;
    QStringList mServerCapabilities;
    QStringList mMailboxes;
    KIMAP::Session *mSession = nullptr;
    QString mSeparator;
    QList <KIMAP::MailBoxDescriptor > mSharedNamespace;
    QList <KIMAP::MailBoxDescriptor > mUserNamespace;

private Q_SLOTS:
    void onGetMetaDataDone(KJob *job);
    void onRightsReceived(KJob *job);
};

#endif
