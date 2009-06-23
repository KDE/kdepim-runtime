/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "dbconsole.h"
#include "dbaccess.h"

#include <KDebug>
#include <KGlobalSettings>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQueryModel>

DbConsole::DbConsole(QWidget* parent) :
  QWidget( parent ),
  mQueryModel( 0 )
{
  ui.setupUi( this );

  ui.execButton->setIcon( KIcon( "application-x-executable" ) );
  connect( ui.execButton, SIGNAL(clicked()), SLOT(execClicked()) );

  ui.queryEdit->setFont( KGlobalSettings::fixedFont() );
  ui.errorView->setFont( KGlobalSettings::fixedFont() );
}

void DbConsole::execClicked()
{
  const QString query = ui.queryEdit->toPlainText();
  if ( query.isEmpty() )
    return;
  delete mQueryModel;
  mQueryModel = new QSqlQueryModel( this );
  mQueryModel->setQuery( query, DbAccess::database() );
  ui.resultView->setModel( mQueryModel );

  if ( mQueryModel->lastError().isValid() ) {
    ui.errorView->appendPlainText( mQueryModel->lastError().text() );
    ui.resultStack->setCurrentWidget( ui.errorViewPage );
  } else {
    ui.errorView->clear();
    ui.resultStack->setCurrentWidget( ui.resultViewPage );
  }
}

#include "dbconsole.moc"
