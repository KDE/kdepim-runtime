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

#ifndef NEWMAILNOTIFIERAGENT_H
#define NEWMAILNOTIFIERAGENT_H

#include <akonadi/collection.h> // make sure this is included before QHash, otherwise it wont find the correct qHash implementation for some reason
#include <akonadi/agentbase.h>

#include <QTimer>
#include <QStringList>

namespace Akonadi {
class AgentInstance;
}

namespace KPIMIdentities {
class IdentityManager;
}

class NewMailNotifierAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit NewMailNotifierAgent( const QString &id );

    void showConfigureDialog(qlonglong windowId = 0);

    void setEnableAgent(bool b);
    bool enabledAgent() const;

    void setVerboseMailNotification(bool b);
    bool verboseMailNotification() const;

    void setBeepOnNewMails(bool b);
    bool beepOnNewMails() const;

    void setShowPhoto(bool b);
    bool showPhoto() const;

    void setShowFrom(bool b);
    bool showFrom() const;

    void setShowSubject(bool b);
    bool showSubject() const;

    void setShowFolderName(bool b);
    bool showFolderName() const;

    void setExcludeMyselfFromNotification(bool b);
    bool excludeMyselfFromNotification() const;

    void setTextToSpeakEnabled(bool enabled);
    bool textToSpeakEnabled() const;

    QString textToSpeak() const;
    void setTextToSpeak(const QString &msg);

    void printDebug();

    bool showButtonToDisplayMail() const;
    void setShowButtonToDisplayMail(bool b);

protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemsMoved( const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection );
    void itemsRemoved( const Akonadi::Item::List &items );
    void itemsFlagsChanged( const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags );
    void doSetOnline(bool online);

private slots:
    void slotShowNotifications();
    void configure(WId windowId);
    void slotInstanceStatusChanged(const Akonadi::AgentInstance &instance);
    void slotInstanceRemoved(const Akonadi::AgentInstance &instance);
    void slotInstanceAdded(const Akonadi::AgentInstance &instance);
    void slotDisplayNotification(const QPixmap &pixmap, const QString &message);
    void slotIdentitiesChanged();
    void slotInstanceNameChanged(const Akonadi::AgentInstance &instance);

private:
    bool ignoreStatusMail(const Akonadi::Item &item);
    bool isActive() const;
    void clearAll();
    bool excludeSpecialCollection(const Akonadi::Collection &collection) const;
    QStringList mListEmails;
    QHash<Akonadi::Collection, QList<Akonadi::Item::Id> > mNewMails;
    QHash<QString, QString> mCacheResourceName;
    QTimer mTimer;
    QStringList mInstanceNameInProgress;
    KPIMIdentities::IdentityManager *mIdentityManager;
};

#endif
