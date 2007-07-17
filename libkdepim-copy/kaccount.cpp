/*  -*- c++ -*-
    kaccount.cpp

    This file is part of KMail, the KDE mail client.

    Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include <kconfiggroup.h>
#include <klocale.h>

KAccount::KAccount( const uint id, const QString &name, const Type type )
  : mId( id ), mName( name ), mType( type )
{
}

void KAccount::writeConfig( KConfigGroup &config ) const
{
  config.writeEntry("Id", mId);
  config.writeEntry("Name", mName);
}

void KAccount::readConfig( const KConfigGroup &config )
{
  mId = config.readEntry("Id", 0);
  mName = config.readEntry("Name");
}

QString KAccount::typeName() const
{
  switch ( type() )
  {
    case Imap: return i18n("IMAP"); break;
    case MBox: return i18n("MBOX"); break;
    case Maildir: return i18n("Maildir"); break;
    case News: return i18n("News"); break;
    case DImap: return i18n("Disconnected IMAP"); break;
    case Local: return i18n("Local"); break;
    case Pop: return i18n("POP"); break;
    case Other:
    default:
      return i18n("Unknown");
  }
}
