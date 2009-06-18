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
#include "storecollectionmodel.h"

#include "akonadi/kcal/kcalmimetypevisitor.h"

#include <kdebug.h>
#include <kdialog.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>

using namespace KCal;

ResourceAkonadiConfig::ResourceAkonadiConfig( QWidget *parent )
  : ResourceConfigBase( QStringList() << QLatin1String( "text/calendar" ), parent )
{
  const QString sourcesTitle = i18nc( "@title:window", "Manage Calendar Sources" );
  mSourcesDialog->setCaption( sourcesTitle );
  mSourcesButton->setText( sourcesTitle );

  mInfoTextLabel->setText( i18nc( "@info",
                                  "<para>By default you will be asked where to put a "
                                  "new Event, Todo or Journal when you create them.</para>"
                                  "<para>For convenience it is also possible to configure "
                                  "a default folder for each of the three data items.</para>"
                                  "<para><note>If the folder list below is empty, you might "
                                  "have to add a calendar source through "
                                  "<interface>%1</interface></note></para>",
                                  sourcesTitle ) );

  mItemTypes[ Akonadi::KCalMimeTypeVisitor::eventMimeType() ] =
    i18nc( "@item:inlistbox, calendar entries", "Events" );
  mItemTypes[ Akonadi::KCalMimeTypeVisitor::todoMimeType() ] =
    i18nc( "@item:inlistbox, calendar entries", "Todos" );
  mItemTypes[ Akonadi::KCalMimeTypeVisitor::journalMimeType() ] =
    i18nc( "@item:inlistbox, calendar entries", "Journals" );

  QCheckBox *checkBox = new QCheckBox( mButtonBox );
  mButtonBox->addButton( checkBox, QDialogButtonBox::ApplyRole );
  checkBox->setText( mItemTypes[ Akonadi::KCalMimeTypeVisitor::eventMimeType() ] );
  mMimeCheckBoxes.insert( Akonadi::KCalMimeTypeVisitor::eventMimeType(), checkBox );
  checkBox->setEnabled( false );

  checkBox = new QCheckBox( mButtonBox );
  mButtonBox->addButton( checkBox, QDialogButtonBox::ApplyRole );
  checkBox->setText( mItemTypes[ Akonadi::KCalMimeTypeVisitor::todoMimeType() ] );
  mMimeCheckBoxes.insert( Akonadi::KCalMimeTypeVisitor::todoMimeType(), checkBox );
  checkBox->setEnabled( false );

  checkBox = new QCheckBox( mButtonBox );
  mButtonBox->addButton( checkBox, QDialogButtonBox::ApplyRole );
  checkBox->setText( mItemTypes[ Akonadi::KCalMimeTypeVisitor::journalMimeType() ] );
  mMimeCheckBoxes.insert( Akonadi::KCalMimeTypeVisitor::journalMimeType(), checkBox );
  checkBox->setEnabled( false );

  connectMimeCheckBoxes();
}

#include "resourceakonadiconfig.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
