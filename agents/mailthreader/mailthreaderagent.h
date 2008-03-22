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

#include <akonadi/agentbase.h>

#include <QList>

using namespace Akonadi;

class MailThreaderAttribute : public Attribute
{
  public:
    MailThreaderAttribute() : Attribute() {}
    MailThreaderAttribute* clone() const {
      MailThreaderAttribute *a =  new MailThreaderAttribute();
      a->mData = mData;
      return a;
    }

    QByteArray type() const { return "MailThreaderSort"; }
    QByteArray toByteArray() const { return mData; }

    void setData( const QByteArray &data ) { mData = data; }
  private:
    QByteArray mData;
};



/**
 * This agent works on a mail collection and tries to thread
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

    static const QLatin1String PartPerfectParents;
    static const QLatin1String PartUnperfectParents;
    static const QLatin1String PartSubjectParents;

    void threadCollection( const Akonadi::Collection &col );

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemRemoved( const Akonadi::Item &item );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    void findParentAndMark( const Akonadi::Item &item );
  private:
    class Private;
    Private* const d;
};

#endif
