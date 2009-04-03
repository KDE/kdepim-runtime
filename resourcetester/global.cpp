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

class GlobalPrivate
{
  public:
    QString basePath;
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
