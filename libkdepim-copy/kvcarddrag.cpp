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

#include "kvcarddrag.h"

#include <kabc/vcardconverter.h>

static const char vcard_mime_string[] = "text/x-vcard";

KVCardDrag::KVCardDrag( const QString &content, QWidget *dragsource, 
                        const char *name )
  : QStoredDrag( vcard_mime_string, dragsource, name )
{
  setVCard( content );
}

KVCardDrag::KVCardDrag( QWidget *dragsource, const char *name )
  : QStoredDrag( vcard_mime_string, dragsource, name )
{
  setVCard( QString::null );
}

void KVCardDrag::setVCard( const QString &content )
{
  setEncodedData( content.utf8() );
}

bool KVCardDrag::canDecode( QMimeSource *e )
{
  return e->provides( vcard_mime_string );
}

bool KVCardDrag::decode( QMimeSource *e, QString &content )
{
  content = QString::fromUtf8( e->encodedData( vcard_mime_string ) );
  return true;
}

bool KVCardDrag::decode( QMimeSource *e, KABC::Addressee::List& addressees )
{
  addressees = KABC::VCardConverter().parseVCards( e->encodedData( vcard_mime_string ) );
  return true;
}

void KVCardDrag::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }

#include "kvcarddrag.moc"
