/*  -*- c++ -*-
  kaccount.cpp

  This file is part of KMail, the KDE mail client.

  Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>
  Copyright (C) 2007 Thomas McGuire <mcguire@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt.  If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.
*/

#include "kaccount.h"

#include <KConfigGroup>
#include <KLocale>

#include <QMetaEnum>

using namespace KPIM;

KAccount::KAccount( const uint id, const QString &name, const Type type )
  : mId( id ), mName( name ), mType( type )
{
}

void KAccount::writeConfig( KConfigGroup &config ) const
{
  config.writeEntry( "Id", mId );
  config.writeEntry( "Name", mName );
}

void KAccount::readConfig( const KConfigGroup &config )
{
  mId = config.readEntry( "Id", 0 );
  mName = config.readEntry( "Name" );
}

QString KAccount::nameForType( Type type )
{
  int index = staticMetaObject.indexOfEnumerator( "Type" );
  return staticMetaObject.enumerator( index ).valueToKey( type );
}

KAccount::Type KAccount::typeForName( const QString &name )
{
  int index = staticMetaObject.indexOfEnumerator( "Type" );
  return static_cast<Type>( staticMetaObject.enumerator( index ).keyToValue(
                              name.toLatin1().constData() ) );
}

QString KAccount::displayNameForType( const Type aType )
{
  switch ( aType ) {
  case Imap:
    return i18nc( "@item IMAP account", "IMAP" );
  case MBox:
    return i18nc( "@item mbox account", "MBOX" );
  case Maildir:
    return i18nc( "@item maildir account", "Maildir mailbox" );
  case News:
    return i18nc( "@item usenet account", "News" );
  case DImap:
    return i18nc( "@item DIMAP account", "Disconnected IMAP" );
  case Local:
    return i18nc( "@item local mailbox account", "Local mailbox" );
  case Pop:
    return i18nc( "@item pop3 account", "POP3" );
  case Other:
  default:
    return i18nc( "@item unknown mail account", "Unknown" );
  }
}

#include "kaccount.moc"
