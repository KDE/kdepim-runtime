/*
    Copyright (C) 2008    Dmitry Ivanov <vonami@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RSSITEMSYNC
#define RSSITEMSYNC

#include <akonadi/itemsync.h>

class RssItemSync : public Akonadi::ItemSync
{
public:
    explicit RssItemSync( const Akonadi::Collection& collection, QObject *parent = 0 );

    void setSynchronizeFlags( bool synchronizeFlags );
    bool synchronizeFlags() const;

protected:
    bool updateItem( const Akonadi::Item& storedItem, Akonadi::Item& newItem );

private:
    bool m_synchronizeFlags;
};

#endif // RSSITEMSYNC
