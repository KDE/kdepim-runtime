/*
   This file is part of libkdepim.

   Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>

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

#include "sidebarextension.h"

#include <qwidget.h>
#include <kparts/part.h>

using namespace KParts;

SideBarExtension::SideBarExtension(QWidget *exported, KParts::ReadOnlyPart *parent, const char* name)
: QObject(parent, name), m_exported(exported), d(0)
{
}

SideBarExtension::~SideBarExtension()
{
}

#include "sidebarextension.moc"

// vim: ts=2 sw=2 et
