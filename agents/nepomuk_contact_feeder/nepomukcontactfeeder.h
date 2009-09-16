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

#include <nepomukfeederagent.h>
#include <akonadi/agentbase.h>
#include <akonadi/item.h>

namespace Soprano
{
class NRLModel;
}

namespace Akonadi {

class NepomukContactFeeder : public NepomukFeederAgent
{
  Q_OBJECT
  public:
    NepomukContactFeeder( const QString &id );
    ~NepomukContactFeeder();

  private:
    void updateItem( const Akonadi::Item &item );
    void updateContactItem( const Akonadi::Item &item, const QUrl& );
    void updateGroupItem( const Akonadi::Item &item, const QUrl& );

    Soprano::NRLModel *mNrlModel;
};

}

#endif
