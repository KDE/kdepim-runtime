/*
    This file is part of libkpimexchange.
    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>

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

#include <typeinfo>

#include <qlabel.h>
#include <qlayout.h>

#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include "vcaldrag.h"
#include "vcalformat.h"
#include "icalformat.h"
#include "resourcelocalconfig.h"
#include "resourcelocal.h"

using namespace KCal;

ResourceLocalConfig::ResourceLocalConfig( QWidget* parent,  const char* name )
    : KRES::ResourceConfigWidget( parent, name )
{
  resize( 245, 115 ); 
  QGridLayout *mainLayout = new QGridLayout( this, 2, 2 );

  QLabel *label = new QLabel( i18n( "Location:" ), this );
  mURL = new KURLRequester( this );
  mainLayout->addWidget( label, 1, 0 );
  mainLayout->addWidget( mURL, 1, 1 );

  formatGroup = new QButtonGroup( 1, Horizontal, i18n( "Calendar Format" ), this );
//  formatGroup->setExclusive( true );

//  formatGroup->setFrameStyle(QFrame::NoFrame);
  icalButton = new QRadioButton( i18n("iCalendar"), formatGroup );
  vcalButton = new QRadioButton( i18n("vCalendar"), formatGroup );

  mainLayout->addWidget( formatGroup, 2, 1 );
}

void ResourceLocalConfig::loadSettings( KRES::Resource *resource )
{
  ResourceLocal* res = dynamic_cast<ResourceLocal*>( resource );
  if ( res ) {
    mURL->setURL( res->mURL.prettyURL() );
    kdDebug() << "Format typeid().name(): " << typeid( res->mFormat ).name() << endl;
    if ( typeid( *(res->mFormat) ) == typeid( ICalFormat ) )
      formatGroup->setButton( 0 );
    else if ( typeid( *(res->mFormat) ) == typeid( VCalFormat ) )
      formatGroup->setButton( 1 );
    else 
      kdDebug() << "ERROR: ResourceLocalConfig::loadSettings(): Unknown format type" << endl;
  } else
    kdDebug(5700) << "ERROR: ResourceLocalConfig::loadSettings(): no ResourceLocal, cast failed" << endl;
}

void ResourceLocalConfig::saveSettings( KRES::Resource *resource )
{
  ResourceLocal* res = dynamic_cast<ResourceLocal*>( resource );
  if (res) {
    res->mURL = mURL->url();

    delete res->mFormat;
    if ( icalButton->isOn() ) {
      res->mFormat = new ICalFormat();
    } else {
      res->mFormat = new VCalFormat();
    }
  } else
    kdDebug(5700) << "ERROR: ResourceLocalConfig::saveSettings(): no ResourceLocal, cast failed" << endl;
}

#include "resourcelocalconfig.moc"
