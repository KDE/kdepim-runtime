/*
 *   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
 *   Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef KMINDEXREADER_SUPPORT
#define KMINDEXREADER_SUPPORT

#include <KShortcut>

#include <QColor>
#include <QFont>
#include <QObject>

class KMIndexTag
{
  public:
    int priority;
    QString tagName;
    QColor backgroundColor;
    QColor textColor;
    QFont textFont;
    bool inToolbar;
    QString iconName;
    KShortcut shortcut;
    bool mEmpty;
};
#endif
