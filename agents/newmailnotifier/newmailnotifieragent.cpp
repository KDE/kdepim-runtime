/*
    Copyright (c) 2013 Laurent Montel <montel@kde.org>

    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "newmailnotifieragent.h"

#include "util.h"

#include "newmailnotifierattribute.h"
#include "specialnotifierjob.h"
#include "newmailnotifieradaptor.h"
#include "newmailnotifieragentsettings.h"
#include "newmailnotifiersettingsdialog.h"

#include <KPIMIdentities/IdentityManager>

#include <akonadi/dbusconnectionpool.h>

#include <akonadi/agentfactory.h>
#include <akonadi/changerecorder.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/session.h>
#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionFetchScope>
#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/kmime/messagestatus.h>
#include <Akonadi/AgentManager>
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>
#include <KNotifyConfigWidget>
#include <KIconLoader>
#include <KIcon>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KWindowSystem>

using namespace Akonadi;

NewMailNotifierAgent::NewMailNotifierAgent( const QString &id )
    : AgentBase( id )
{
    Akonadi::AttributeFactory::registerAttribute<NewMailNotifierAttribute>();
    new NewMailNotifierAdaptor( this );

    mIdentityManager = new KPIMIdentities::IdentityManager( false, this );
    connect(mIdentityManager, SIGNAL(changed()), SLOT(slotIdentitiesChanged()));
    slotIdentitiesChanged();

    DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/NewMailNotifierAgent" ),
                                                           this, QDBusConnection::ExportAdaptors );
    DBusConnectionPool::threadConnection().registerService( QLatin1String( "org.freedesktop.Akonadi.NewMailNotifierAgent" ) );

    connect( Akonadi::AgentManager::self(), SIGNAL(instanceStatusChanged(Akonadi::AgentInstance)),
             this, SLOT(slotInstanceStatusChanged(Akonadi::AgentInstance)) );
    connect( Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)),
             this, SLOT(slotInstanceRemoved(Akonadi::AgentInstance)) );
    connect( Akonadi::AgentManager::self(), SIGNAL(instanceAdded(Akonadi::AgentInstance)),
             this, SLOT(slotInstanceAdded(Akonadi::AgentInstance)) );
    connect( Akonadi::AgentManager::self(), SIGNAL(instanceNameChanged(Akonadi::AgentInstance)),
             this, SLOT(slotInstanceNameChanged(Akonadi::AgentInstance)) );


    changeRecorder()->setMimeTypeMonitored( KMime::Message::mimeType() );
    changeRecorder()->itemFetchScope().setCacheOnly( true );
    changeRecorder()->itemFetchScope().setFetchModificationTime( false );
    changeRecorder()->fetchCollection( true );
    changeRecorder()->setChangeRecordingEnabled( false );
    changeRecorder()->ignoreSession( Akonadi::Session::defaultSession() );
    changeRecorder()->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );
    changeRecorder()->setCollectionMonitored(Collection::root(), true);
    mTimer.setInterval( 5 * 1000 );
    connect( &mTimer, SIGNAL(timeout()), SLOT(slotShowNotifications()) );

    if (NewMailNotifierAgentSettings::textToSpeakEnabled())
        Util::testJovieService();

    if (isActive()) {
        mTimer.setSingleShot( true );
    }
}

void NewMailNotifierAgent::slotIdentitiesChanged()
{
    mListEmails = mIdentityManager->allEmails();
}

void NewMailNotifierAgent::doSetOnline(bool online)
{
    if (!online) {
        clearAll();
    }
}

void NewMailNotifierAgent::setExcludeMyselfFromNotification(bool b)
{
    NewMailNotifierAgentSettings::setExcludeEmailsFromMe(b);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::excludeMyselfFromNotification() const
{
    return NewMailNotifierAgentSettings::excludeEmailsFromMe();
}

void NewMailNotifierAgent::setShowPhoto(bool show)
{
    NewMailNotifierAgentSettings::setShowPhoto(show);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::showPhoto() const
{
    return NewMailNotifierAgentSettings::showPhoto();
}

void NewMailNotifierAgent::setShowFrom(bool show)
{
    NewMailNotifierAgentSettings::setShowFrom(show);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::showFrom() const
{
    return NewMailNotifierAgentSettings::showFrom();
}

void NewMailNotifierAgent::setShowSubject(bool show)
{
    NewMailNotifierAgentSettings::setShowSubject(show);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::showSubject() const
{
    return NewMailNotifierAgentSettings::showSubject();
}

void NewMailNotifierAgent::setShowFolderName(bool show)
{
    NewMailNotifierAgentSettings::setShowFolder(show);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::showFolderName() const
{
    return NewMailNotifierAgentSettings::showFolder();
}

void NewMailNotifierAgent::setEnableAgent(bool enabled)
{
    NewMailNotifierAgentSettings::setEnabled(enabled);
    NewMailNotifierAgentSettings::self()->writeConfig();
    if (!enabled) {
        clearAll();
    }
}

void NewMailNotifierAgent::setVerboseMailNotification(bool verbose)
{
    NewMailNotifierAgentSettings::setVerboseNotification(verbose);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::verboseMailNotification() const
{
    return NewMailNotifierAgentSettings::verboseNotification();
}

void NewMailNotifierAgent::setBeepOnNewMails(bool beep)
{
    NewMailNotifierAgentSettings::setBeepOnNewMails(beep);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::beepOnNewMails() const
{
    return NewMailNotifierAgentSettings::beepOnNewMails();
}

void NewMailNotifierAgent::setTextToSpeakEnabled(bool enabled)
{
    NewMailNotifierAgentSettings::setTextToSpeakEnabled(enabled);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

bool NewMailNotifierAgent::textToSpeakEnabled() const
{
    return NewMailNotifierAgentSettings::textToSpeakEnabled();
}

void NewMailNotifierAgent::setTextToSpeak(const QString &msg)
{
    NewMailNotifierAgentSettings::setTextToSpeak(msg);
    NewMailNotifierAgentSettings::self()->writeConfig();
}

QString NewMailNotifierAgent::textToSpeak() const
{
    return NewMailNotifierAgentSettings::textToSpeak();
}

void NewMailNotifierAgent::clearAll()
{
    mNewMails.clear();
    mInstanceNameInProgress.clear();
}

bool NewMailNotifierAgent::enabledAgent() const
{
    return NewMailNotifierAgentSettings::enabled();
}

bool NewMailNotifierAgent::showButtonToDisplayMail() const
{
    return NewMailNotifierAgentSettings::showButtonToDisplayMail();
}

void NewMailNotifierAgent::setShowButtonToDisplayMail(bool b)
{
    NewMailNotifierAgentSettings::setShowButtonToDisplayMail(b);
    NewMailNotifierAgentSettings::self()->writeConfig();
}


void NewMailNotifierAgent::showConfigureDialog(qlonglong windowId)
{
    configure( windowId );
}

void NewMailNotifierAgent::configure( WId windowId )
{
    QPointer<NewMailNotifierSettingsDialog> dialog = new NewMailNotifierSettingsDialog;
    if (windowId) {
#ifndef Q_WS_WIN
        KWindowSystem::setMainWindow( dialog, windowId );
#else
        KWindowSystem::setMainWindow( dialog, (HWND)windowId );
#endif
    }
    dialog->exec();
    delete dialog;
}

bool NewMailNotifierAgent::excludeSpecialCollection(const Akonadi::Collection &collection) const
{
    if ( collection.hasAttribute<Akonadi::EntityHiddenAttribute>() )
        return true;

    if ( collection.hasAttribute<NewMailNotifierAttribute>() ) {
        if (collection.attribute<NewMailNotifierAttribute>()->ignoreNewMail()) {
            return true;
        }
    }

    if (!collection.contentMimeTypes().contains( KMime::Message::mimeType()) ) {
        return true;
    }

    SpecialMailCollections::Type type = SpecialMailCollections::self()->specialCollectionType(collection);
    switch(type) {
    case SpecialMailCollections::Invalid: //Not a special collection
    case SpecialMailCollections::Inbox:
        return false;
    default:
        return true;
    }

}

void NewMailNotifierAgent::itemsRemoved(const Item::List &items )
{
    if (!isActive())
        return;

    QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator end(mNewMails.end());
    for ( QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator it = mNewMails.begin(); it != end; ++it ) {
        QList<Akonadi::Item::Id> idList = it.value();
        bool itemFound = false;
        Q_FOREACH( const Item &item, items ) {
            if (idList.contains(item.id())) {
                idList.removeAll( item.id() );
                itemFound = true;
            }
        }
        if (itemFound) {
            if (mNewMails[it.key()].isEmpty()) {
                mNewMails.remove( it.key() );
            } else {
                mNewMails[it.key()] = idList;
            }
        }
    }
}

void NewMailNotifierAgent::itemsFlagsChanged( const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags )
{
    if (!isActive())
        return;
    Q_FOREACH (const Akonadi::Item &item, items) {
        QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator end(mNewMails.end());
        for ( QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator it = mNewMails.begin(); it != end; ++it ) {
            QList<Akonadi::Item::Id> idList= it.value();
            if (idList.contains(item.id()) && addedFlags.contains("\\SEEN")) {
                idList.removeAll( item.id() );
                if ( idList.isEmpty() ) {
                    mNewMails.remove( it.key() );
                    break;
                } else {
                    (*it) = idList;
                }
            }
        }
    }
}

void NewMailNotifierAgent::itemsMoved( const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination )
{
    if (!isActive())
        return;

    Q_FOREACH (const Akonadi::Item &item, items) {
        if (ignoreStatusMail(item)) {
            continue;
        }

        if ( excludeSpecialCollection(collectionSource) ) {
            continue; // outbox, sent-mail, trash, drafts or templates.
        }

        if ( mNewMails.contains( collectionSource ) ) {
            QList<Akonadi::Item::Id> idListFrom = mNewMails[ collectionSource ];
            if ( idListFrom.contains( item.id() ) ) {
                idListFrom.removeAll( item.id() );

                if ( idListFrom.isEmpty() ) {
                    mNewMails.remove( collectionSource );
                } else {
                    mNewMails[ collectionSource ] = idListFrom;
                }
                if ( !excludeSpecialCollection(collectionDestination) ) {
                    QList<Akonadi::Item::Id> idListTo = mNewMails[ collectionDestination ];
                    idListTo.append( item.id() );
                    mNewMails[ collectionDestination ] = idListTo;
                }
            }
        }
    }
}

bool NewMailNotifierAgent::ignoreStatusMail(const Akonadi::Item &item)
{
    Akonadi::MessageStatus status;
    status.setStatusFromFlags( item.flags() );
    if ( status.isRead() || status.isSpam() || status.isIgnored() )
        return true;
    return false;
}

void NewMailNotifierAgent::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
    if (!isActive())
        return;

    if ( excludeSpecialCollection(collection) ) {
        return; // outbox, sent-mail, trash, drafts or templates.
    }

    if (ignoreStatusMail(item)) {
        return;
    }

    if ( !mTimer.isActive() ) {
        mTimer.start();
    }
    mNewMails[ collection ].append( item.id() );
}

void NewMailNotifierAgent::slotShowNotifications()
{
    if (mNewMails.isEmpty())
        return;

    if (!isActive())
        return;

    if (!mInstanceNameInProgress.isEmpty()) {
        //Restart timer until all is done.
        mTimer.start();
        return;
    }

    QString message;
    if (NewMailNotifierAgentSettings::verboseNotification()) {
        bool hasUniqMessage = true;
        Akonadi::Item::Id item = -1;
        QString currentPath;
        QStringList texts;
        QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::const_iterator end(mNewMails.constEnd());
        const int numberOfCollection(mNewMails.count());
        if (numberOfCollection > 1)
            hasUniqMessage = false;

        for ( QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::const_iterator it = mNewMails.constBegin(); it != end; ++it ) {
            Akonadi::EntityDisplayAttribute *attr = it.key().attribute<Akonadi::EntityDisplayAttribute>();
            QString displayName;
            if ( attr && !attr->displayName().isEmpty() )
                displayName = attr->displayName();
            else
                displayName = it.key().name();

            if (hasUniqMessage) {
                if (it.value().count() == 0) {
                    //You can have an unique folder with 0 message
                    return;
                } else if (it.value().count() == 1 ) {
                    item = it.value().first();
                    currentPath = displayName;
                    break;
                } else {
                    hasUniqMessage = false;
                }
            }
            QString resourceName;
            if (!mCacheResourceName.contains(it.key().resource())) {
                Q_FOREACH ( const Akonadi::AgentInstance &instance, Akonadi::AgentManager::self()->instances() ) {
                    if (instance.identifier() == it.key().resource()) {
                        mCacheResourceName.insert(instance.identifier(), instance.name());
                        resourceName = instance.name();
                        break;
                    }
                }
            } else {
                resourceName = mCacheResourceName.value(it.key().resource());
            }
            const int numberOfEmails(it.value().count());
            if (numberOfEmails>0) {
                texts.append( i18np( "One new email in %2 from \"%3\"", "%1 new emails in %2 from \"%3\"", numberOfEmails, displayName,
                                 resourceName ) );
            }
        }
        if (hasUniqMessage) {
            SpecialNotifierJob *job = new SpecialNotifierJob(mListEmails, currentPath, item, this);
            connect(job, SIGNAL(displayNotification(QPixmap,QString)), SLOT(slotDisplayNotification(QPixmap,QString)));
            mNewMails.clear();
            return;
        } else {
            message = texts.join( QLatin1String("<br>") );
        }
    } else {
        message = i18n( "New mail arrived" );
    }

    kDebug() << message;

    slotDisplayNotification(Util::defaultPixmap(), message);

    mNewMails.clear();
}


void NewMailNotifierAgent::slotDisplayNotification(const QPixmap &pixmap, const QString &message)
{
    Util::showNotification(pixmap, message);

    if ( NewMailNotifierAgentSettings::beepOnNewMails() ) {
        KNotification::beep();
    }
}

void NewMailNotifierAgent::slotInstanceNameChanged(const Akonadi::AgentInstance &instance)
{
    if (!isActive())
        return;

    const QString identifier(instance.identifier());
    if (mCacheResourceName.contains(identifier)) {
        mCacheResourceName.remove(identifier);
        mCacheResourceName.insert(identifier, instance.name());
    }
}

void NewMailNotifierAgent::slotInstanceStatusChanged(const Akonadi::AgentInstance &instance)
{
    if (!isActive())
        return;

    const QString identifier(instance.identifier());
    switch(instance.status()) {
    case Akonadi::AgentInstance::Broken:
    case Akonadi::AgentInstance::Idle:
    {
        if (mInstanceNameInProgress.contains(identifier)) {
            mInstanceNameInProgress.removeAll(identifier);
        }
        break;
    }
    case Akonadi::AgentInstance::Running:
    {
        if (!Util::excludeAgentType(instance)) {
            if (!mInstanceNameInProgress.contains(identifier)) {
                mInstanceNameInProgress.append(identifier);
            }
        }
        break;
    }
    case Akonadi::AgentInstance::NotConfigured:
        //Nothing
        break;
    }
}

void NewMailNotifierAgent::slotInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    if (!isActive())
        return;

    const QString identifier(instance.identifier());
    if (mInstanceNameInProgress.contains(identifier)) {
        mInstanceNameInProgress.removeAll(identifier);
    }
}

void NewMailNotifierAgent::slotInstanceAdded(const Akonadi::AgentInstance &instance)
{
    mCacheResourceName.insert(instance.identifier(), instance.name());
}

void NewMailNotifierAgent::printDebug()
{
    kDebug()<<"instance in progress: "<<mInstanceNameInProgress
            <<"\n notifier enabled : "<<NewMailNotifierAgentSettings::enabled()
            <<"\n check in progress : "<<!mInstanceNameInProgress.isEmpty()
            <<"\n beep on new mails: "<<NewMailNotifierAgentSettings::beepOnNewMails();
}

bool NewMailNotifierAgent::isActive() const
{
    return isOnline() && NewMailNotifierAgentSettings::enabled();
}

AKONADI_AGENT_MAIN( NewMailNotifierAgent )


