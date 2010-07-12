/*
 * Copyright (c) 2009 Volker Krause <vkrause@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "global.h"

#include <kglobal.h>
#include <QDir>
#include <kstandarddirs.h>
#include <KDebug>

class GlobalPrivate
{
  public:
    GlobalPrivate() : parent( new QObject() ) {}
    QString basePath;
    QString vmPath;
    QObject *parent;
};

K_GLOBAL_STATIC( GlobalPrivate, sInstance )

QString Global::basePath()
{
  return sInstance->basePath;
}

void Global::setBasePath(const QString& path)
{
  sInstance->basePath = path;
  if ( !path.endsWith( QDir::separator() ) )
    sInstance->basePath += QDir::separator();
}

QString Global::vmPath()
{
  if ( sInstance->vmPath.isEmpty() )
    setVMPath( KStandardDirs::locateLocal( "cache", "akonadi-resourcetester/", true ) );
  return sInstance->vmPath;
}

void Global::setVMPath(const QString& path)
{
  kDebug() << path;
  sInstance->vmPath = path;
  if ( !path.endsWith( QDir::separator() ) )
    sInstance->vmPath += QDir::separator();
  const QDir dir( path );
  if ( !dir.exists() )
    QDir::root().mkpath( path );
}

QObject* Global::parent()
{
  Q_ASSERT( sInstance->parent );
  return sInstance->parent;
}

void Global::cleanup()
{
  delete sInstance->parent;
  sInstance->parent = 0;
}
