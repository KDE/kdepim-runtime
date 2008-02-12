/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <tomalbers@kde.nl>

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

#ifndef IMAPLIB_CONFIGMODULE_H
#define IMAPLIB_CONFIGMODULE_H

#define KDE3_SUPPORT
#include <kcmodule.h>
#undef KDE3_SUPPORT

/**
  KCModule for transport management.
*/
class ConfigModule : public KCModule
{
  public:
    explicit ConfigModule( QWidget *parent = 0,
                           const QVariantList &args = QVariantList() );
};

#endif
