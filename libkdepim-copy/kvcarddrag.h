/*
    This file is part of the KDE PIM library.
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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KVCARDDRAG_H
#define KVCARDDRAG_H

#include <qdragobject.h>
#include <qstring.h>

class KVCardDragPrivate;

/**
 * A drag-and-drop object for vcards. The according MIME type
 * is set to text/x-vcard.
 *
 * See the Qt drag'n'drop documentation.
 */
class KVCardDrag : public QStoredDrag
{
  Q_OBJECT

  public:
    /**
     * Constructs an empty vcard drag.
     */
    KVCardDrag( QWidget *dragsource = 0, const char *name = 0 );

    /**
     * Constructs a vcard drag with the @p addressee.
     */
    KVCardDrag( const QString &content, QWidget *dragsource = 0, const char *name = 0 );
    virtual ~KVCardDrag() {};

    /**
     * Sets the vcard of the drag to @p content.
     */
    void setVCard( const QString &content );

    /**
     * Returns true if the MIME source @p e contains a vcard object.
     */
    static bool canDecode( QMimeSource *e );

    /**
     * Decodes the MIME source @p e and puts the resulting vcard into @p content.
     */
    static bool decode( QMimeSource *e, QString &content );
 
    /**
     * Decodes the MIME source @p e and puts the resulting vcard into @p addresseess.
     */
    static bool decode( QMimeSource *e, KABC::Addressee::List& addressees );

  protected:
     virtual void virtual_hook( int id, void* data );

  private:
     KVCardDragPrivate *d;
};

#endif // KVCARDDRAG_H
