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

#include <KGlobalSettings>

#include <QByteArray>


NoteDisplayAttribute::NoteDisplayAttribute()
    : Akonadi::Attribute(),
      mFont(KGlobalSettings::generalFont()),
      mTitleFont(KGlobalSettings::windowTitleFont()),
      mBackgroundColor(Qt::yellow),
      mForegroundgroundColor(Qt::black),
      mSize(300,300),
      mPosition(QPoint( -10000, -10000 )),
      mTabSize(4),
      mDesktop(-10),
      mRememberDesktop(true),
      mAutoIndent(true),
      mHide(false),
      mShowInTaskbar(false),
      mKeepAbove(false),
      mKeepBelow(false)
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

int NoteDisplayAttribute::tabSize() const
{
    return mTabSize;
}

void NoteDisplayAttribute::setTabSize(int value)
{
    mTabSize = value;
}

bool NoteDisplayAttribute::autoIndent() const
{
    return mAutoIndent;
}

void NoteDisplayAttribute::setAutoIndent(bool b)
{
    mAutoIndent = b;
}

void NoteDisplayAttribute::setFont(const QFont &f)
{
    mFont = f;
}

QFont NoteDisplayAttribute::font() const
{
    return mFont;
}

void NoteDisplayAttribute::setTitleFont(const QFont &f)
{
    mTitleFont = f;
}

QFont NoteDisplayAttribute::titleFont() const
{
    return mTitleFont;
}

void NoteDisplayAttribute::setDesktop(int v)
{
    mDesktop = v;
}

int NoteDisplayAttribute::desktop() const
{
    return mDesktop;
}

void NoteDisplayAttribute::setIsHidden(bool b)
{
    mHide = b;
}

bool NoteDisplayAttribute::isHidden() const
{
    return mHide;
}

void NoteDisplayAttribute::setPosition(const QPoint &pos)
{
    mPosition = pos;
}

QPoint NoteDisplayAttribute::position() const
{
    return mPosition;
}

void NoteDisplayAttribute::setShowInTaskbar(bool b)
{
    mShowInTaskbar = b;
}

bool NoteDisplayAttribute::showInTaskbar() const
{
    return mShowInTaskbar;
}

void NoteDisplayAttribute::setKeepAbove(bool b)
{
    mKeepAbove = b;
}

bool NoteDisplayAttribute::keepAbove() const
{
    return mKeepAbove;
}

void NoteDisplayAttribute::setKeepBelow(bool b)
{
    mKeepBelow = b;
}

bool NoteDisplayAttribute::keepBelow() const
{
    return mKeepBelow;
}
