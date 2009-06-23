/*
    This file is part of Akonadi.

    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#include "searchdialog.h"

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QTextEdit>

SearchDialog::SearchDialog( QWidget *parent )
  : KDialog( parent )
{
  setCaption( "Create Search" );
  setButtons( Ok | Cancel );

  QWidget *widget = new QWidget( this );
  setMainWidget( widget );

  QGridLayout *layout = new QGridLayout( widget );

  mName = new QLineEdit;
  mName->setText( "My Search" );
  mEdit = new QTextEdit;
  mEdit->setAcceptRichText( false );
  mEdit->setWhatsThis( "Enter a SparQL query here" );

  layout->addWidget( new QLabel( "Search query name:" ), 0, 0 );
  layout->addWidget( mName, 0, 1 );
  layout->addWidget( mEdit, 1, 0, 1, 2 );
}

SearchDialog::~SearchDialog()
{
}

QString SearchDialog::searchName() const
{
  return mName->text();
}

QString SearchDialog::searchQuery() const
{
  return mEdit->toPlainText();
}
