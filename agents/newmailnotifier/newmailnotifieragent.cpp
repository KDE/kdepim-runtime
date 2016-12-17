/*
    Copyright (c) 2013-2015 Laurent Montel <montel@kde.org>

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

#include <AkonadiCore/NewMailNotifierAttribute>
#include "specialnotifierjob.h"
#include "newmailnotifieradaptor.h"
#include "newmailnotifieragentsettings.h"
#include "newmailnotifiersettingsdialog.h"

#include <KIdentityManagement/IdentityManager>

#include <kdbusconnectionpool.h>

#include <changerecorder.h>
#include <entitydisplayattribute.h>
#include <entityhiddenattribute.h>
#include <itemfetchscope.h>
#include <session.h>
#include <AttributeFactory>
#include <CollectionFetchScope>
#include <Akonadi/KMime/SpecialMailCollections>
#include <Akonadi/KMime/MessageStatus>
#include <AgentManager>
#include <KLocalizedString>
#include <KMime/Message>
#include <KNotification>
#include <KWindowSystem>
#include <Kdelibs4ConfigMigrator>
#include "newmailnotifier_debug.h"
#include <KToolInvocation>
#include <QIcon>
#include <KIconLoader>
#ifdef HAVE_TEXTTOSPEECH
#include <QTextToSpeech>
#endif

using namespace Akonadi;

NewMailNotifierAgent::NewMailNotifierAgent(const QString &id)
    : AgentBase(id)
#ifdef HAVE_TEXTTOSPEECH
    , mTextToSpeech(Q_NULLPTR)
#endif
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("newmailnotifieragent"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi_newmailnotifier_agentrc") << QStringLiteral("akonadi_newmailnotifier_agent.notifyrc"));
    migrate.migrate();

    KLocalizedString::setApplicationDomain("akonadi_newmailnotifier_agent");
    Akonadi::AttributeFactory::registerAttribute<Akonadi::NewMailNotifierAttribute>();
    new NewMailNotifierAdaptor(this);

    mIdentityManager = new KIdentityManagement::IdentityManager(true, this);
    connect(mIdentityManager, SIGNAL(changed()), SLOT(slotIdentitiesChanged()));
    slotIdentitiesChanged();
    mDefaultPixmap = QIcon::fromTheme(QStringLiteral("kmail")).pixmap(KIconLoader::SizeMedium, KIconLoader::SizeMedium);

    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/NewMailNotifierAgent"),
            this, QDBusConnection::ExportAdaptors);
    KDBusConnectionPool::threadConnection().registerService(QStringLiteral("org.freedesktop.Akonadi.NewMailNotifierAgent"));

    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceStatusChanged, this, &NewMailNotifierAgent::slotInstanceStatusChanged);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceRemoved, this, &NewMailNotifierAgent::slotInstanceRemoved);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceAdded, this, &NewMailNotifierAgent::slotInstanceAdded);
    connect(Akonadi::AgentManager::self(), &Akonadi::AgentManager::instanceNameChanged, this, &NewMailNotifierAgent::slotInstanceNameChanged);

    changeRecorder()->setMimeTypeMonitored(KMime::Message::mimeType());
    changeRecorder()->itemFetchScope().setCacheOnly(true);
    changeRecorder()->itemFetchScope().setFetchModificationTime(false);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->setChangeRecordingEnabled(false);
    changeRecorder()->ignoreSession(Akonadi::Session::defaultSession());
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
    changeRecorder()->setCollectionMonitored(Collection::root(), true);
    mTimer.setInterval(5 * 1000);
    connect(&mTimer, &QTimer::timeout, this, &NewMailNotifierAgent::slotShowNotifications);

    if (isActive()) {
        mTimer.setSingleShot(true);
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
    NewMailNotifierAgentSettings::self()->save();
}

bool NewMailNotifierAgent::excludeMyselfFromNotification() const
{
    return NewMailNotifierAgentSettings::excludeEmailsFromMe();
}

void NewMailNotifierAgent::setShowPhoto(bool show)
{
    NewMailNotifierAgentSettings::setShowPhoto(show);
    NewMailNotifierAgentSettings::self()->save();
}

bool NewMailNotifierAgent::showPhoto() const
{
    return NewMailNotifierAgentSettings::showPhoto();
}

void NewMailNotifierAgent::setShowFrom(bool show)
{
    NewMailNotifierAgentSettings::setShowFrom(show);
    NewMailNotifierAgentSettings::self()->save();
}

bool NewMailNotifierAgent::showFrom() const
{
    return NewMailNotifierAgentSettings::showFrom();
}

void NewMailNotifierAgent::setShowSubject(bool show)
{
    NewMailNotifierAgentSettings::setShowSubject(show);
    NewMailNotifierAgentSettings::self()->save();
}

bool NewMailNotifierAgent::showSubject() const
{
    return NewMailNotifierAgentSettings::showSubject();
}

void NewMailNotifierAgent::setShowFolderName(bool show)
{
    NewMailNotifierAgentSettings::setShowFolder(show);
    NewMailNotifierAgentSettings::self()->save();
}

bool NewMailNotifierAgent::showFolderName() const
{
    return NewMailNotifierAgentSettings::showFolder();
}

void NewMailNotifierAgent::setEnableAgent(bool enabled)
{
    NewMailNotifierAgentSettings::setEnabled(enabled);
    NewMailNotifierAgentSettings::self()->save();
    if (!enabled) {
        clearAll();
    }
}

void NewMailNotifierAgent::setVerboseMailNotification(bool verbose)
{
    NewMailNotifierAgentSettings::setVerboseNotification(verbose);
    NewMailNotifierAgentSettings::self()->save();
}

bool NewMailNotifierAgent::verboseMailNotification() const
{
    return NewMailNotifierAgentSettings::verboseNotification();
}

void NewMailNotifierAgent::setTextToSpeakEnabled(bool enabled)
{
    NewMailNotifierAgentSettings::setTextToSpeakEnabled(enabled);
    NewMailNotifierAgentSettings::self()->save();
}

bool NewMailNotifierAgent::textToSpeakEnabled() const
{
    return NewMailNotifierAgentSettings::textToSpeakEnabled();
}

void NewMailNotifierAgent::setTextToSpeak(const QString &msg)
{
    NewMailNotifierAgentSettings::setTextToSpeak(msg);
    NewMailNotifierAgentSettings::self()->save();
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
    NewMailNotifierAgentSettings::self()->save();
}

void NewMailNotifierAgent::showConfigureDialog(qlonglong windowId)
{
#ifndef Q_OS_WIN
    configure(static_cast<WId>(windowId));
#else
    configure(reinterpret_cast<WId>(windowId));
#endif
}

void NewMailNotifierAgent::configure(WId windowId)
{
    QPointer<NewMailNotifierSettingsDialog> dialog = new NewMailNotifierSettingsDialog;
    if (windowId) {
#ifndef Q_OS_WIN
        KWindowSystem::setMainWindow(dialog, windowId);
#else
        KWindowSystem::setMainWindow(dialog, (HWND)windowId);
#endif
    }
    dialog->exec();
    delete dialog;
}

bool NewMailNotifierAgent::excludeSpecialCollection(const Akonadi::Collection &collection) const
{
    if (collection.hasAttribute<Akonadi::EntityHiddenAttribute>()) {
        return true;
    }

    if (collection.hasAttribute<Akonadi::NewMailNotifierAttribute>()) {
        if (collection.attribute<Akonadi::NewMailNotifierAttribute>()->ignoreNewMail()) {
            return true;
        }
    }

    if (!collection.contentMimeTypes().contains(KMime::Message::mimeType())) {
        return true;
    }

    SpecialMailCollections::Type type = SpecialMailCollections::self()->specialCollectionType(collection);
    switch (type) {
    case SpecialMailCollections::Invalid: //Not a special collection
    case SpecialMailCollections::Inbox:
        return false;
    default:
        return true;
    }

}

void NewMailNotifierAgent::itemsRemoved(const Item::List &items)
{
    if (!isActive()) {
        return;
    }

    QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator end(mNewMails.end());
    for (QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator it = mNewMails.begin(); it != end; ++it) {
        QList<Akonadi::Item::Id> idList = it.value();
        bool itemFound = false;
        for (const Item &item : items) {
            if (idList.contains(item.id())) {
                idList.removeAll(item.id());
                itemFound = true;
            }
        }
        if (itemFound) {
            if (mNewMails[it.key()].isEmpty()) {
                mNewMails.remove(it.key());
            } else {
                mNewMails[it.key()] = idList;
            }
        }
    }
}

void NewMailNotifierAgent::itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags)
{
    Q_UNUSED(removedFlags);

    if (!isActive()) {
        return;
    }
    for (const Akonadi::Item &item : items) {
        QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator end(mNewMails.end());
        for (QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::iterator it = mNewMails.begin(); it != end; ++it) {
            QList<Akonadi::Item::Id> idList = it.value();
            if (idList.contains(item.id()) && addedFlags.contains("\\SEEN")) {
                idList.removeAll(item.id());
                if (idList.isEmpty()) {
                    mNewMails.remove(it.key());
                    break;
                } else {
                    (*it) = idList;
                }
            }
        }
    }
}

void NewMailNotifierAgent::itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination)
{
    if (!isActive()) {
        return;
    }

    for (const Akonadi::Item &item : items) {
        if (ignoreStatusMail(item)) {
            continue;
        }

        if (excludeSpecialCollection(collectionSource)) {
            continue; // outbox, sent-mail, trash, drafts or templates.
        }

        if (mNewMails.contains(collectionSource)) {
            QList<Akonadi::Item::Id> idListFrom = mNewMails[ collectionSource ];
            if (idListFrom.contains(item.id())) {
                idListFrom.removeAll(item.id());

                if (idListFrom.isEmpty()) {
                    mNewMails.remove(collectionSource);
                } else {
                    mNewMails[ collectionSource ] = idListFrom;
                }
                if (!excludeSpecialCollection(collectionDestination)) {
                    QList<Akonadi::Item::Id> idListTo = mNewMails[ collectionDestination ];
                    idListTo.append(item.id());
                    mNewMails[ collectionDestination ] = idListTo;
                }
            }
        }
    }
}

bool NewMailNotifierAgent::ignoreStatusMail(const Akonadi::Item &item)
{
    Akonadi::MessageStatus status;
    status.setStatusFromFlags(item.flags());
    if (status.isRead() || status.isSpam() || status.isIgnored()) {
        return true;
    }
    return false;
}

void NewMailNotifierAgent::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!isActive()) {
        return;
    }

    if (excludeSpecialCollection(collection)) {
        return; // outbox, sent-mail, trash, drafts or templates.
    }

    if (ignoreStatusMail(item)) {
        return;
    }

    if (!mTimer.isActive()) {
        mTimer.start();
    }
    mNewMails[ collection ].append(item.id());
}

void NewMailNotifierAgent::slotShowNotifications()
{
    if (mNewMails.isEmpty()) {
        return;
    }

    if (!isActive()) {
        return;
    }

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
        if (numberOfCollection > 1) {
            hasUniqMessage = false;
        }

        for (QHash< Akonadi::Collection, QList<Akonadi::Item::Id> >::const_iterator it = mNewMails.constBegin(); it != end; ++it) {
            Akonadi::EntityDisplayAttribute *attr = it.key().attribute<Akonadi::EntityDisplayAttribute>();
            QString displayName;
            if (attr && !attr->displayName().isEmpty()) {
                displayName = attr->displayName();
            } else {
                displayName = it.key().name();
            }

            if (hasUniqMessage) {
                const int numberOfValue(it.value().count());
                if (numberOfValue == 0) {
                    //You can have an unique folder with 0 message
                    return;
                } else if (numberOfValue == 1) {
                    item = it.value().at(0);
                    currentPath = displayName;
                    break;
                } else {
                    hasUniqMessage = false;
                }
            }
            QString resourceName;
            if (!mCacheResourceName.contains(it.key().resource())) {
                Q_FOREACH (const Akonadi::AgentInstance &instance, Akonadi::AgentManager::self()->instances()) {
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
            if (numberOfEmails > 0) {
                texts.append(i18ncp("%2 = name of mail folder; %3 = name of Akonadi POP3/IMAP/etc resource (as user named it)",
                                    "One new email in %2 from \"%3\"",
                                    "%1 new emails in %2 from \"%3\"", numberOfEmails, displayName,
                                    resourceName));
            }
        }
        if (hasUniqMessage) {
            SpecialNotifierJob *job = new SpecialNotifierJob(mListEmails, currentPath, item, this);
            job->setDefaultPixmap(mDefaultPixmap);
            connect(job, &SpecialNotifierJob::displayNotification, this, &NewMailNotifierAgent::slotDisplayNotification);
#ifdef HAVE_TEXTTOSPEECH
            connect(job, &SpecialNotifierJob::say, this, &NewMailNotifierAgent::slotSay);
#endif
            mNewMails.clear();
            return;
        } else {
            message = texts.join(QStringLiteral("<br>"));
        }
    } else {
        message = i18n("New mail arrived");
    }

    qCDebug(NEWMAILNOTIFIER_LOG) << message;

    slotDisplayNotification(mDefaultPixmap, message);

    mNewMails.clear();
}

void NewMailNotifierAgent::slotDisplayNotification(const QPixmap &pixmap, const QString &message)
{
    KNotification::event(QStringLiteral("new-email"),
                         message,
                         pixmap,
                         Q_NULLPTR,
                         KNotification::CloseOnTimeout,
                         QStringLiteral("akonadi_newmailnotifier_agent"));

}

void NewMailNotifierAgent::slotInstanceNameChanged(const Akonadi::AgentInstance &instance)
{
    if (!isActive()) {
        return;
    }

    const QString identifier(instance.identifier());
    if (mCacheResourceName.contains(identifier)) {
        mCacheResourceName.remove(identifier);
        mCacheResourceName.insert(identifier, instance.name());
    }
}

void NewMailNotifierAgent::slotInstanceStatusChanged(const Akonadi::AgentInstance &instance)
{
    if (!isActive()) {
        return;
    }

    const QString identifier(instance.identifier());
    switch (instance.status()) {
    case Akonadi::AgentInstance::Broken:
    case Akonadi::AgentInstance::Idle: {
        if (mInstanceNameInProgress.contains(identifier)) {
            mInstanceNameInProgress.removeAll(identifier);
        }
        break;
    }
    case Akonadi::AgentInstance::Running: {
        if (!excludeAgentType(instance)) {
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

bool NewMailNotifierAgent::excludeAgentType(const Akonadi::AgentInstance &instance)
{
    if (instance.type().mimeTypes().contains(KMime::Message::mimeType())) {
        const QStringList capabilities(instance.type().capabilities());
        if (capabilities.contains(QStringLiteral("Resource")) &&
                !capabilities.contains(QStringLiteral("Virtual")) &&
                !capabilities.contains(QStringLiteral("MailTransport"))) {
            return false;
        } else {
            return true;
        }
    }
    return true;
}

void NewMailNotifierAgent::slotInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    if (!isActive()) {
        return;
    }

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
    qCDebug(NEWMAILNOTIFIER_LOG) << "instance in progress: " << mInstanceNameInProgress
                                 << "\n notifier enabled : " << NewMailNotifierAgentSettings::enabled()
                                 << "\n check in progress : " << !mInstanceNameInProgress.isEmpty();
}

bool NewMailNotifierAgent::isActive() const
{
    return isOnline() && NewMailNotifierAgentSettings::enabled();
}

void NewMailNotifierAgent::slotSay(const QString &message)
{
#ifdef HAVE_TEXTTOSPEECH
    if (!mTextToSpeech) {
        mTextToSpeech = new QTextToSpeech(this);
    }
    if (mTextToSpeech->availableEngines().isEmpty()) {
        qCWarning(NEWMAILNOTIFIER_LOG) << "No texttospeech engine available";
    } else {
        mTextToSpeech->say(message);
    }
#endif
}

AKONADI_AGENT_MAIN(NewMailNotifierAgent)
