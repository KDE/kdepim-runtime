/**
 * kemeditor.cpp
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

#include "kmeditor.h"
#include "kmeditor.moc"
#include "kemailquotinghighter.h"

class KMeditor::Private
{
  public:
    Private()
     : useExtEditor(false)
    {
    }

    ~Private()
    {
    }
   
    QString extEditorPath;
    bool useExtEditor;
};

KMeditor::KMeditor( const QString& text, QWidget *parent)
 : KTextEdit(text, parent), d( new Private() )
{
}

KMeditor::KMeditor( QWidget *parent)
 : KTextEdit(parent), d( new Private() )
{
}

KMeditor::~KMeditor()
{
  delete d;
  d = 0;
}

void KMeditor::createHighlighter()
{
   KSpell2::KEMailQuotingHighlighter *emailHighLighter = new KSpell2::KEMailQuotingHighlighter(this);
   //TODO change config
   setHightighter(emailHighLighter);
}


void KMeditor::setUseExternalEditor( bool use )
{
   d->useExtEditor = use;
}

void KMeditor::setExternalEditorPath( const QString & path )
{
  d->extEditorPath = path;   
}


