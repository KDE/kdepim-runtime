// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
/**
 * plugin.cpp
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
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
#include "plugin.h"

#include "core.h"

#include <kdebug.h>
#include <QString>

namespace Komposer
{

class Plugin::Private
{
public:
  //Core* core;
};

Plugin::Plugin( QObject *parent, const char *name, const QStringList & )
    : QObject( parent, name ), d( new Private )
{
  //d->core = core;
}

Plugin::~Plugin()
{
  delete d; d = 0;
}

void
Plugin::startedComposing()
{
}

void
Plugin::sendClicked()
{
}

void
Plugin::quitClicked()
{
}

void
Plugin::aboutToUnload()
{
  kDebug()<<"plugin unloading";
  emit readyForUnload();
}

Core*
Plugin::core() const
{
  return 0;
  //return d->core;
}

}//end namespace Komposer

#include "plugin.moc"
