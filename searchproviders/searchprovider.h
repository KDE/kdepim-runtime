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

#include <QList>
#include <QObject>

namespace Akonadi {

/**
  Abstract interface for search providers.
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
     * Asynchronously searches for all items that match the search query and
     * stores them in the target collection.
     *
     * Returns an unique id used to identify this search request when the
     * searchResultAvailable() signal is emitted.
     *
     * @param targetCollection The collection the search results are stored in.
     * @param searchQuery The search query.
     */
    virtual int search( const QString &targetCollection, const QString &searchQuery ) = 0;

    /**
     * Asynchronously starts the generation of a fetch response
     * for the given list of item ids.
     *
     * Returns an unique id used to identify this search request when the
     * searchResultAvailable() signal is emitted.
     *
     * @param uids List of item ids
     * @param field Fetch response field.
     */
    virtual int fetchResponse( const QList<int> uids, const QString &field ) = 0;

  Q_SIGNALS:
    /**
     * Emitted when a search operation has been finished.
     * @param ticket Used to identify the corresponding search request.
     */
    void searchResultAvailable( int ticket );

    /**
     * Emitted when a fetch response generation has been finished.
     * @param ticket Used to identify the corresponding searcg request.
     * @param result A list containing the requested fetch response in the
     * same order the uids were given in fetchResponse().
     */
    void fetchResponseAvailable( int ticket, const QStringList &result );
};

}

#endif
