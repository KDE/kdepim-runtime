/*
    This file is part of libkabc.
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
#include "storecollectionmodel.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <kdebug.h>
#include <kdialog.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>

using namespace KABC;

ResourceAkonadiConfig::ResourceAkonadiConfig( QWidget *parent )
  : ResourceConfigBase( QStringList() << Addressee::mimeType()
                                      << ContactGroup::mimeType(),
                        parent )
{
  const QString sourcesTitle = i18nc( "@title:window", "Manage Address Book Sources" );
  mSourcesDialog->setCaption( sourcesTitle );
  mSourcesButton->setText( sourcesTitle );

  mInfoTextLabel->setText( i18nc( "@info",
                                  "<title>Please select the folder for storing"
                                   " newly created contacts.</title><note>If the folder"
                                  " list below is empty, you might have to add an"
                                  " address book source through <interface>%1</interface></note>",
                                  sourcesTitle ) );

  mItemTypes[ Addressee::mimeType() ] = i18nc( "@item:inlistbox, address book entries", "Contacts" );
  mItemTypes[ ContactGroup::mimeType() ] = i18nc( "@item:inlistbox, email distribution lists", "Distribution Lists" );

  QCheckBox *checkBox = new QCheckBox( mButtonBox );
  mButtonBox->addButton( checkBox, QDialogButtonBox::ApplyRole );
  checkBox->setText( mItemTypes[ Addressee::mimeType() ] );
  mMimeCheckBoxes.insert( Addressee::mimeType(), checkBox );
  checkBox->setEnabled( false );

  checkBox = new QCheckBox( mButtonBox );
  mButtonBox->addButton( checkBox, QDialogButtonBox::ApplyRole );
  checkBox->setText( mItemTypes[ ContactGroup::mimeType() ] );
  mMimeCheckBoxes.insert( ContactGroup::mimeType(), checkBox );
  checkBox->setEnabled( false );

  connectMimeCheckBoxes();
}

#include "resourceakonadiconfig.moc"
