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

class NewMailNotifierAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    explicit NewMailNotifierAgent( const QString &id );


    void setEnableNotifier(bool b);
    bool enabledNotifier() const;

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


    void printDebug();

protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination );
    void itemRemoved( const Akonadi::Item &item );
    void itemChanged( const Akonadi::Item &, const QSet< QByteArray > &);
    void doSetOnline(bool online);

private slots:
    void slotShowNotifications();
    void configure(WId windowId);
    void slotInstanceStatusChanged(const Akonadi::AgentInstance &instance);
    void slotInstanceRemoved(const Akonadi::AgentInstance &instance);
    void slotDisplayNotification(const QPixmap &pixmap, const QString &message);

private:
    bool isActive() const;
    void clearAll();
    bool excludeSpecialCollection(const Akonadi::Collection &collection) const;
    QHash<Akonadi::Collection, QList<Akonadi::Item::Id> > mNewMails;
    QTimer mTimer;
    QStringList mInstanceNameInProgress;
};

#endif
