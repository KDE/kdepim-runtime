/*
    This file is part of the KDE PIM library.
    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
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

  protected:
     virtual void virtual_hook( int id, void* data );

  private:
     KVCardDragPrivate *d;
};

#endif // KVCARDDRAG_H
