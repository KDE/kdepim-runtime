/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AKONADISEARCHPROVIDER_H
#define AKONADISEARCHPROVIDER_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QDBusMessage>

namespace Akonadi {

/**
  Abstract interface for search providers.
  It's recommended to use SearchProviderBase as an base class for your search
  provider though.
*/
class SearchProvider : public QObject
{
  Q_OBJECT

  public:
    virtual ~SearchProvider() {};

    /**
     * Returns a list of supported mimetypes.
     */
    virtual QList<QByteArray> supportedMimeTypes() const = 0;

    /**
     * Searches for all items that match the search query and
     * stores them in the target collection.
     *
     * The search results are kept up-to-date until the search is removed
     * with removeSearch() again.
     *
     * @param targetCollection The collection the search results are stored in.
     * @param searchQuery The search query.
     * @param msg DBus message needed for delayed replys.
     */
    virtual bool addSearch( const QString &targetCollection, const QString &searchQuery, const QDBusMessage &msg ) = 0;

    /**
     * Stop updating a persistent search specified by the given collection.
     *
     * @param collection The collection representing the corresponding persistent search.
     */
    virtual bool removeSearch( const QString &collection ) = 0;

    /**
     * Generates a fetch response for the given list of item ids.
     *
     * @param uids List of item ids
     * @param field Fetch response field.
     * @param msg DBus message needed for delayed replys.
     */
    // ### FIXME: QList<int> not yet supported by the qdbus bindings!
    virtual QStringList fetchResponse( const QList<QString> uids, const QString &field, const QDBusMessage &msg ) = 0;

    /**
     * Terminate the search provider.
    */
    virtual void quit() = 0;
};

}

#endif
