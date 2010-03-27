/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Tom Albers <toma@kde.org>

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

#include "providerpage.h"
#include "global.h"

#include <KDebug>
#include <QSortFilterProxyModel>
#include <knewstuff3/downloaddialog.h>

ProviderPage::ProviderPage(KAssistantDialog* parent) :
  Page( parent ),
  m_model( new QStandardItemModel( this ) )
{
  ui.setupUi( this );

  QSortFilterProxyModel *proxy = new QSortFilterProxyModel( this );
  proxy->setSourceModel( m_model );
  ui.listView->setModel( proxy );
  ui.searchLine->setProxy( proxy );

  connect( ui.listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectionChanged()) );

  connect( ui.ghnsButton, SIGNAL( clicked() ), SLOT( ghnsClicked() ) );
  kDebug();
}

void ProviderPage::ghnsClicked()
{
  kDebug();
  KNS3::DownloadDialog dialog( this );
  dialog.exec();
  foreach (const KNS3::Entry e, dialog.installedEntries()) {
    kDebug() << "Changed Entry: " << e.name();
    
    QStandardItem *item = new QStandardItem( e.name() );
    item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    item->setData( e.name(), Qt::ToolTipRole );
    m_model->appendRow( item );

    //KNS3::Entry::Entry() is private :(
    //m_providerEntries[ item ] = e;
  }
}

void ProviderPage::selectionChanged()
{
  if ( ui.listView->selectionModel()->hasSelection() ) {
    setValid( true );
  } else {
    setValid( false );
  }
}

void ProviderPage::leavePageNext()
{
  if ( !ui.listView->selectionModel()->hasSelection() )
    return;
  const QModelIndex index = ui.listView->selectionModel()->selectedIndexes().first();
  const QStandardItem* item =  m_model->itemFromIndex(index);

  // download and execute it...
  //KNS3::Entry::Entry() is private though :(
  //const KNS3::Entry entry( m_providerEntries[item] );
  //Global::setAssistant( entry.name() );
}

QTreeView *ProviderPage::treeview() const
{
  return ui.listView;
}

QPushButton *ProviderPage::advancedButton() const
{
  return ui.advancedButton;
}

#include "providerpage.moc"
