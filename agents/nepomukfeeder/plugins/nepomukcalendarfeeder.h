/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>
                  2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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


#ifndef NEPOMUKCALENDARFEEDER_H
#define NEPOMUKCALENDARFEEDER_H

#include <nepomukfeederplugin.h>
#include <KCalCore/Journal>
#include <KCalCore/Event>
#include <KCalCore/Todo>

namespace Akonadi {

class NepomukCalendarFeeder: public NepomukFeederPlugin
{
  Q_OBJECT
  Q_INTERFACES( Akonadi::NepomukFeederPlugin )
public:
  NepomukCalendarFeeder( QObject *parent, const QVariantList &): NepomukFeederPlugin(parent ){};
  virtual void updateItem( const Akonadi::Item& item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph );
private:
  void updateEventItem( const Akonadi::Item& item, const KCalCore::Event::Ptr&, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );
  void updateJournalItem( const Akonadi::Item& item, const KCalCore::Journal::Ptr&, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );
  void updateTodoItem( const Akonadi::Item& item, const KCalCore::Todo::Ptr&, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );

  void updateIncidenceItem( const KCalCore::Incidence::Ptr &calInc, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );
};

}

#endif // NEPOMUKNOTEFEEDER_H
