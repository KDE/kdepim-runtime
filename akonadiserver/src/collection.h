/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef AKONADICOLLECTION_H
#define AKONADICOLLECTION_H

#include <QList>
#include <QString>

#include "global.h"

namespace Akonadi {

/**
  @brief A collection of objects.
 */
class Collection {
public:
    Collection( const QString & identifier );

    ~Collection();

    QString identifier() const;

    bool isNoSelect() const;
    bool isNoInferiors() const;

private:
    QString m_identifier;
    bool m_noSelect;
    bool m_noInferiors;
};

typedef QList<Collection> CollectionList;
typedef QListIterator<Collection> CollectionListIterator;

}

#endif
