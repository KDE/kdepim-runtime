/*
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

#include <QtCore/QTimer>

class NewMailNotifierAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV2
{
  Q_OBJECT

  public:
    NewMailNotifierAgent( const QString &id );
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

  private slots:
    void showNotifications();

  private:
    QHash<Akonadi::Collection, int> m_newMails;
    QTimer m_timer;
};

#endif
