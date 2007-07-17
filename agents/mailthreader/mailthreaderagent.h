/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#ifndef MAILTHREADERAGENT_H
#define MAILTHREADERAGENT_H

#include <libakonadi/agentbase.h>

#include <QList>

using namespace Akonadi;

/**
 * This agent works on a mail collection an tries to thread
 * mails related to the same discussion :
 * 
 * Mail
 * + Reply1
 *   + Reply11
 *   + Reply12
 * + Reply2
 * ...
 */
class MailThreaderAgent : public Akonadi::AgentBase
{
  Q_OBJECT

  public:
    MailThreaderAgent( const QString &id );
    ~MailThreaderAgent();

    static const QLatin1String PartParent;
    static const QLatin1String PartSort;

    void setCollection( const Akonadi::Collection &col );

  public Q_SLOTS:
    virtual void configure();
    void threadCollection();
    QList<DataReference> childrenOf( const Item& item );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QStringList &parts );
    virtual void itemRemoved( const Akonadi::DataReference &ref );

    void findParentAndMark( const Akonadi::Item &item );
  private:
    class Private;
    Private* const d;
};

#endif
