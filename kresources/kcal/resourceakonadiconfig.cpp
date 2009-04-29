/*
    This file is part of libkcal.
    Copyright (c) 2008, 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include <QLabel>
#include <QPushButton>

using namespace KCal;

ResourceAkonadiConfig::ResourceAkonadiConfig( QWidget *parent )
  : ResourceConfigBase( QStringList() << QLatin1String( "text/calendar" ), parent )
{
  const QString sourcesTitle = i18nc( "@title:window", "Manage Address Book Sources" );
  mSourcesDialog->setCaption( sourcesTitle );
  mSourcesButton->setText( sourcesTitle );

  mInfoTextLabel->setText( i18nc( "@info",
                                  "<title>Please select the folder for storing"
                                  " newly created calendar entries.</title><note>If the folder"
                                  " list below is empty, you might have to add a"
                                  " calendar through <interface>%1</interface>"
                                  " <br>or change which folders you are subscribed to"
                                  " through <interface>%2</interface></note>",
                                  sourcesTitle, mSubscriptionButton->text() ) );
}

#include "resourceakonadiconfig.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
