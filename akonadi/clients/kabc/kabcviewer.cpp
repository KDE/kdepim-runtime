/*
    This file is part of Akonadi.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "kabcviewer.h"

#include <QtGui/QVBoxLayout>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "kabc/kabcitembrowser.h"

Dialog::Dialog( QWidget *parent )
  : KDialog( parent )
{
  setCaption( "Contact Viewer" );
  setButtons( Close );
  showButtonSeparator( true );

  QWidget *wdg = new QWidget( this );
  QVBoxLayout *layout = new QVBoxLayout( wdg );

  mBrowser = new Akonadi::KABCItemBrowser( wdg );
  layout->addWidget( mBrowser );

  setMainWidget( wdg );

  resize( 520, 580 );
}

Dialog::~Dialog()
{
}

void Dialog::loadUid( Akonadi::Item::Id uid )
{
  mBrowser->setItem( Akonadi::Item( uid ) );
}

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, "kabcviewer", 0, ki18n("KABC Viewer"), "1.0" , ki18n("A contact viewer for Akonadi"));

  KCmdLineOptions options;
  options.add("uid <uid>", ki18n( "Uid of the Akonadi contact" ));
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  Dialog dlg;
  if ( !args->isSet( "uid" ) ) {
    KCmdLineArgs::usage();
    return 1;
  }

  dlg.loadUid( args->getOption( "uid" ).toLongLong() );
  dlg.exec();

  return 0;
}

#include "kabcviewer.moc"
