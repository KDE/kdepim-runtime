/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_STRIGI_FEEDER_H
#define AKONADI_STRIGI_FEEDER_H

#include <akonadi/agentbase.h>
#include <strigi/qtdbus/strigiclient.h>

namespace Akonadi {

/**
  Full text search provider using strigi.
*/
class StrigiFeeder : public AgentBase, public AgentBase::Observer
{
  Q_OBJECT

  public:
    StrigiFeeder( const QString &id );

  protected:
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers );
    void itemRemoved(const Akonadi::Item &item);

  private:
    StrigiClient mStrigi;
};

}

#endif
