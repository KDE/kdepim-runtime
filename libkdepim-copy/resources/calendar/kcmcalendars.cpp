/*
    This file is part of libkabc.
    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>

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
/*
#include <qgroupbox.h>
#include <qinputdialog.h>
#include <qlabel.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
*/
// #include "resource.h"
// #include "resourceconfigdlg.h"
// #include "resourcefactory.h"

#include <qlayout.h>

#include <resourcesconfigpage.h>
#include "kcmcalendars.h"

KCMCalendars::KCMCalendars( QWidget *parent, const char *name )
  : KCModule( parent, name )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  mConfigPage = new KRES::ResourcesConfigPage( "calendar", this );
  layout->addWidget( mConfigPage );
  connect( mConfigPage, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );
}

void KCMCalendars::load()
{
  mConfigPage->load();
}

void KCMCalendars::save()
{
  mConfigPage->save();
}

void KCMCalendars::defaults()
{
  mConfigPage->defaults();
}

extern "C"
{
  KCModule *create_kcalendars( QWidget *parent, const char * ) {
    return new KCMCalendars( parent, "kcalendars" );
  }
}

#include "kcmcalendars.moc"
