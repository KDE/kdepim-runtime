/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "global.h"

#include <kdebug.h>
#include <KGlobal>
#include <KStandardDirs>
#include <QtCore/qfileinfo.h>
#include <QtCore/QDir>

class GlobalPrivate
{
  public:
    QStringList filter;
    QString assistant;
};

K_GLOBAL_STATIC( GlobalPrivate, sInstance )


QString Global::assistant()
{
  return sInstance->assistant;
}

void Global::setAssistant(const QString& assistant)
{
  const QFileInfo info( assistant );
  if ( info.isAbsolute() ) {
    sInstance->assistant = assistant;
    return;
  } 

  const QStringList list = KGlobal::dirs()->findAllResources(
    "data", QLatin1String( "akonadi/accountwizard/*.desktop" ),
    KStandardDirs::Recursive | KStandardDirs::NoDuplicates );
  foreach ( const QString &entry, list ) {
    const QFileInfo info( entry );
    const QDir dir( info.absolutePath() );
    kDebug() << dir.dirName();
    if ( dir.dirName() == assistant ) {
      sInstance->assistant = entry;
      return;
    }
  }

  sInstance->assistant.clear();
}


QStringList Global::typeFilter()
{
  return sInstance->filter;
}

void Global::setTypeFilter(const QStringList& filter)
{
  sInstance->filter = filter;
}

QString Global::assistantBasePath()
{
  const QFileInfo info( assistant() );
  if ( info.isAbsolute() )
    return info.absolutePath() + QDir::separator();
  return QString();
}
