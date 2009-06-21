/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>
                  2008 Sebastian Trueg <trueg@kde.org>

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

#ifndef AKONADI_NEPOMUK_CONTACT_FEEDER_H
#define AKONADI_NEPOMUK_CONTACT_FEEDER_H

#include <akonadi/agentbase.h>
#include <akonadi/item.h>

namespace Soprano
{
class NRLModel;
}

namespace Akonadi {

class NepomukContactFeeder : public AgentBase, public AgentBase::Observer
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.akonadi.NepomukContactFeeder" )

  public:
    NepomukContactFeeder( const QString &id );
    ~NepomukContactFeeder();

  public Q_SLOTS:
    Q_SCRIPTABLE void updateAll( bool force = false );

  protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemRemoved(const Akonadi::Item &item);

  private Q_SLOTS:
    void slotInitialItemScan();
    void slotItemsReceivedForInitialScan( const Akonadi::Item::List& items );

  private:
    void updateItem( const Akonadi::Item &item );

    bool mForceUpdate;
    Soprano::NRLModel *mNrlModel;
};

}

#endif
