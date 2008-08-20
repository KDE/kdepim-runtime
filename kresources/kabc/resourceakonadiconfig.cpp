/*
    This file is part of libkabc.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "resourceakonadiconfig.h"
#include "resourceakonadi.h"

#include <kdebug.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kcmoduleloader.h>

#include <QLayout>

using namespace KABC;

ResourceAkonadiConfig::ResourceAkonadiConfig( QWidget *parent )
  : KRES::ConfigWidget( parent )
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( 0 );
  mainLayout->setSpacing( KDialog::spacingHint() );

  // list of contact data MIME types
  // TODO: check if we need to add text/x-vcard
  QStringList list;
  list << "text/directory";

  QWidget *widget = KCModuleLoader::loadModule( "kcm_akonadi_resources",
                                                KCModuleLoader::Inline, this, list );
  mainLayout->addWidget( widget );
}

void ResourceAkonadiConfig::loadSettings( KRES::Resource *res )
{
  ResourceAkonadi *resource = dynamic_cast<ResourceAkonadi*>( res );

  if ( !resource ) {
    kDebug(5700) << "cast failed";
    return;
  }
}

void ResourceAkonadiConfig::saveSettings( KRES::Resource *res )
{
  ResourceAkonadi *resource = dynamic_cast<ResourceAkonadi*>( res );

  if ( !resource ) {
    kDebug(5700) << "cast failed";
    return;
  }
}

#include "resourceakonadiconfig.moc"
