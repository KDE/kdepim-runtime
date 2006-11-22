/*
    This file is part of libkdepim.

    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KVCARDDRAG_H
#define KVCARDDRAG_H

#include <QByteArray>

#include <kabc/addressee.h>
#include <kdepim_export.h>


/**
 * A drag-and-drop object for vcards. The according MIME type
 * is set to text/x-vcard.
 *
 * See the Qt drag'n'drop documentation.
 */
class KDEPIM_EXPORT KVCardDrag
{
  public:
    /**
      Mime-type of VCard
    */
    static QString mimeType();

    /**
      Adds the VCard representation as data of the drag object
    */
    static bool populateMimeData( QMimeData *md, const QByteArray &content );
    /**
      Adds the VCard representation as data of the drag object
    */
    static bool populateMimeData( QMimeData *md, const KABC::Addressee::List &adressees );
    /**
      Return, if drag&drop object can be decode to iCalendar.
    */
    static bool canDecode( const QMimeData * );
    /**
      Decode drag&drop object to VCard component \a content.
    */
    static bool fromMimeData( const QMimeData *md, QByteArray &content );
    /**
     * Decodes the MIME data @p md and puts the resulting vcard into @p addresseess.
     */
    static bool fromMimeData( const QMimeData *md, KABC::Addressee::List& addressees );

  private:
     class Private;
     Private *d;
};

#endif // KVCARDDRAG_H
