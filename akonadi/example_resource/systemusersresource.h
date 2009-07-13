/*
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef SYSTEMUSERSRESOURCE_H
#define SYSTEMUSERSRESOURCE_H

#include <akonadi/resourcebase.h>

namespace KABC {
  class Addressee;
}

struct passwd;

class SystemUsersResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    SystemUsersResource( const QString &id );
    ~SystemUsersResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void aboutToQuit();

    KABC::Addressee addresseeFromPWEnt( passwd *pwEnt );
};

#endif
