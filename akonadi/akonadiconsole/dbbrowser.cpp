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

#include "dbbrowser.h"
#include "dbaccess.h"

#include <QSqlDatabase>
#include <QSqlTableModel>

DbBrowser::DbBrowser(QWidget* parent) :
  QWidget( parent ),
  mTableModel( 0 )
{
  ui.setupUi( this );

  ui.tableBox->addItems( DbAccess::database().tables(QSql::AllTables) );

  ui.refreshButton->setIcon( KIcon( "view-refresh" ) );
  connect( ui.refreshButton, SIGNAL(clicked()), SLOT(refreshClicked()) );
}

void DbBrowser::refreshClicked()
{
  const QString table = ui.tableBox->currentText();
  if ( table.isEmpty() )
    return;
  delete mTableModel;
  mTableModel = new QSqlTableModel( this, DbAccess::database() );
  mTableModel->setTable( table );
  mTableModel->setEditStrategy( QSqlTableModel::OnRowChange );
  mTableModel->select();
  ui.tableView->setModel( mTableModel );
}

#include "dbbrowser.moc"
