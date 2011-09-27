/*
    Copyright (C) 2009    Dmitry Ivanov <vonami@gmail.com>
    Copyright (C) 2011    Alessandro Cosentino <cosenal@gmail.com>

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

#ifndef UTIL_H
#define UTIL_H

#include "opmlparser.h"

#include <krss/rssitem.h>
#include <Syndication/Item>

namespace Util {
  
    KRss::RssItem fromSyndicationItem( const Syndication::ItemPtr& syndItem );
    QList<boost::shared_ptr<const ParsedNode> > parsedDescendants( QList< Akonadi::Collection >& collections, Akonadi::Collection parent );  
};

#endif // UTIL_H
