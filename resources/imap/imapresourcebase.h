/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef RESOURCES_IMAP_IMAPRESOURCEBASE_H
#define RESOURCES_IMAP_IMAPRESOURCEBASE_H

#include <AkonadiAgentBase/resourcebase.h>
#include <AkonadiAgentBase/agentsearchinterface.h>
#include <QDialog>
#include <QPointer>
#include "resourcestateinterface.h"
#include "resourcestate.h"

class QTimer;

class ResourceTask;
namespace KIMAP
{
class Session;
}

class ImapIdleManager;
class SessionPool;
class ResourceState;
class SubscriptionDialog;
class Settings;

class ImapResourceBase : public Akonadi::ResourceBase,
    public Akonadi::AgentBase::ObserverV4,
    public Akonadi::AgentSearchInterface
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.ImapResourceBase")
protected:
    using Akonadi::AgentBase::Observer::collectionChanged;

public:
    explicit ImapResourceBase(const QString &id);
    ~ImapResourceBase();

    virtual QDialog *createConfigureDialog(WId windowId) = 0;

    void cleanup() Q_DECL_OVERRIDE;

    virtual Settings *settings() const;

public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;

    // DBus methods
    Q_SCRIPTABLE void requestManualExpunge(qint64 collectionId);
    Q_SCRIPTABLE int configureSubscription(qlonglong windowId = 0);
    Q_SCRIPTABLE QStringList serverCapabilities() const;

    // pseudo-virtual called by ResourceBase
    QString dumpResourceToString() const Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void startIdleIfNeeded();
    void startIdle();

    void abortActivity() Q_DECL_OVERRIDE;

    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveCollectionAttributes(const Akonadi::Collection &col) Q_DECL_OVERRIDE;

    void retrieveItems(const Akonadi::Collection &col) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags) Q_DECL_OVERRIDE;
    void itemsRemoved(const Akonadi::Item::List &items) Q_DECL_OVERRIDE;
    virtual void itemsMoved(const Akonadi::Item::List &item, const Akonadi::Collection &source,
                            const Akonadi::Collection &destination) Q_DECL_OVERRIDE;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) Q_DECL_OVERRIDE;
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void collectionRemoved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    virtual void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source,
                                 const Akonadi::Collection &destination) Q_DECL_OVERRIDE;

    void addSearch(const QString &query, const QString &queryLanguage, const Akonadi::Collection &resultCollection) Q_DECL_OVERRIDE;
    void removeSearch(const Akonadi::Collection &resultCollection) Q_DECL_OVERRIDE;
    void search(const QString &query, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;

    void doSetOnline(bool online) Q_DECL_OVERRIDE;

    QChar separatorCharacter() const;
    void setSeparatorCharacter(const QChar &separator);

    void aboutToQuit() Q_DECL_OVERRIDE;

    virtual ResourceStateInterface::Ptr createResourceState(const TaskArguments &);
    virtual QString defaultName() const = 0;

private Q_SLOTS:
    void doSearch(const QVariant &arg);

    void reconnect();

    void scheduleConnectionAttempt();
    void startConnect(const QVariant &);   // the parameter is necessary, since this method is used by the task scheduler
    void onConnectDone(int errorCode, const QString &errorMessage);
    void onConnectionLost(KIMAP::Session *session);

    void onIdleCollectionFetchDone(KJob *job);

    void onExpungeCollectionFetchDone(KJob *job);
    void triggerCollectionExpunge(const QVariant &collectionVariant);

    void taskDestroyed(QObject *task);

    void showError(const QString &message);
    void clearStatusMessage();

    void updateResourceName();

    void onCollectionModifyDone(KJob *job);

protected:
    //Starts and queues a task
    void startTask(ResourceTask *task);
    void queueTask(ResourceTask *task);
    SessionPool *m_pool;
    mutable Settings *m_settings;

    void delayedInit();

private:
    friend class ResourceState;

    bool needsNetwork() const;
    void modifyCollection(const Akonadi::Collection &);

    friend class ImapIdleManager;

    QList<ResourceTask *> m_taskList; //used to be able to kill tasks
    QPointer<SubscriptionDialog> mSubscriptions;
    ImapIdleManager *m_idle;
    QTimer *m_statusMessageTimer;
    QChar m_separatorCharacter;
};

#endif
