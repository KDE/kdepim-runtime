/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "notedisplayattribute.h"

#include <QByteArray>

NoteDisplayAttribute::NoteDisplayAttribute()
    : Akonadi::Attribute(),
      mBackgroundColor(Qt::yellow),
      mForegroundgroundColor(Qt::black),
      mSize(300,300),
      mRememberDesktop(true)
{

}

NoteDisplayAttribute::~NoteDisplayAttribute()
{

}

NoteDisplayAttribute* NoteDisplayAttribute::clone() const
{
    NoteDisplayAttribute *attr = new NoteDisplayAttribute();
    return attr;
}

void NoteDisplayAttribute::deserialize(const QByteArray& data)
{
}

QByteArray NoteDisplayAttribute::serialized() const
{
    //TODO
    return "?";
}

QByteArray NoteDisplayAttribute::type() const
{
    return "NoteDisplayAttribute";
}

void NoteDisplayAttribute::setBackgroundColor(const QColor &color)
{
    mBackgroundColor = color;
}

QColor NoteDisplayAttribute::backgroundColor() const
{
    return mBackgroundColor;
}

void NoteDisplayAttribute::setForegroundColor(const QColor &color)
{
    mForegroundgroundColor = color;
}

QSize NoteDisplayAttribute::size() const
{
    return mSize;
}

void NoteDisplayAttribute::setSize(const QSize &size)
{
    mSize = size;
}

QColor NoteDisplayAttribute::foregroundColor() const
{
    return mForegroundgroundColor;
}

bool NoteDisplayAttribute::rememberDesktop() const
{
    return mRememberDesktop;
}

void NoteDisplayAttribute::setRememberDesktop(bool b)
{
    mRememberDesktop = b;
}
