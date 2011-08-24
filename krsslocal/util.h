/*
    Copyright (C) 2009    Dmitry Ivanov <vonami@gmail.com>

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

#ifndef KRSSRESOURCE_UTIL_H
#define KRSSRESOURCE_UTIL_H

#include <Syndication/Item>

namespace KRss {
class RssItem;
}

namespace Akonadi {
class Collection;
}

namespace KRssResource
{

KRss::RssItem fromSyndicationItem( const Syndication::ItemPtr& syndItem );
//QString generateCollectionName( const Akonadi::Collection& collection );

} // namespace KRssResource

#endif // KRSSRESOURCE_UTIL_H
