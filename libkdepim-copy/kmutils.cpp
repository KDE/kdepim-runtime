/**
 * kmutils.cpp
 *
 * Copyright (C)  2007 Laurent Montel <montel@kde.org>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "kmutils.h"

QString KMUtils::rot13(const QString &s)
{
  QString r(s);

  for (int i=0; i<r.length(); i++) {
    if ( r[i] >= QChar('A') && r[i] <= QChar('M') ||
         r[i] >= QChar('a') && r[i] <= QChar('m') )
      r[i] = (char)((int)QChar(r[i]).toLatin1() + 13);
    else
      if  ( r[i] >= QChar('N') && r[i] <= QChar('Z') ||
            r[i] >= QChar('n') && r[i] <= QChar('z') )
        r[i] = (char)((int)QChar(r[i]).toLatin1() - 13);
  }

  return r;
}

