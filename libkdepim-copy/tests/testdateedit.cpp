/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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

#include <qlayout.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

#include "kdateedit.h"

#include "testdateedit.h"

DateEdit::DateEdit( QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  
  KDateEdit *edit = new KDateEdit( this );
  layout->addWidget( edit );

  connect( edit, SIGNAL( dateChanged( const QDate& ) ),
           this, SLOT( dateChanged( const QDate& ) ) );
}

void DateEdit::dateChanged( const QDate &date )
{
  if ( date.isValid() )
    qDebug( "%s", date.toString().latin1() );
  else
    qDebug( "invalid date entered" );
}

int main(int argc,char **argv)
{
  KAboutData aboutData( "testdateedit", "Test KDateEdit", "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  DateEdit dateEdit;
  app.setMainWidget( &dateEdit );
  dateEdit.show();

  app.exec();
}

#include "testdateedit.moc"
