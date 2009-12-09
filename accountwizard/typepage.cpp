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

#include "typepage.h"

#include <KDebug>
#include <KDesktopFile>
#include <KStandardDirs>

#include <QSortFilterProxyModel>
#include "global.h"

TypePage::TypePage(KAssistantDialog* parent) :
  Page( parent ),
  m_model( new QStandardItemModel( this ) )
{
  ui.setupUi( this );

  QSortFilterProxyModel *proxy = new QSortFilterProxyModel( this );
  proxy->setSourceModel( m_model );
  ui.listView->setModel( proxy );
  ui.searchLine->setProxy( proxy );

  const QStringList list = KGlobal::dirs()->findAllResources( "data", QLatin1String( "akonadi/accountwizard/*.desktop" ),
                                                              KStandardDirs::Recursive | KStandardDirs::NoDuplicates );
  const QStringList filter = Global::typeFilter();
  foreach ( const QString &entry, list ) {
    KDesktopFile f( entry );
    kDebug() << entry << f.readName();
    const KConfig configWizard( entry );
    KConfigGroup grp( &configWizard, "Wizard" );
    if ( !filter.isEmpty() && !filter.contains( grp.readEntry( "Type" ) ) )
      continue;
    QStandardItem *item = new QStandardItem( f.readName() );
    item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    item->setData( entry, Qt::UserRole );
    if ( !f.readIcon().isEmpty() )
      item->setData( KIcon( f.readIcon() ), Qt::DecorationRole );
    item->setData( f.readComment(), Qt::ToolTipRole );
    m_model->appendRow( item );
  }

  connect( ui.listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectionChanged()) );
  connect( ui.listView, SIGNAL(doubleClicked(QModelIndex)), SLOT(slotLeavePageNext()));
}

void TypePage::selectionChanged()
{
  if ( ui.listView->selectionModel()->hasSelection() ) {
    setValid( true );
  } else {
    setValid( false );
  }
}

void TypePage::leavePageNext()
{
  if ( !ui.listView->selectionModel()->hasSelection() )
    return;
  const QModelIndex index = ui.listView->selectionModel()->selectedIndexes().first();
  Global::setAssistant( index.data( Qt::UserRole ).toString() );
}

QTreeView *TypePage::treeview() const
{
  return ui.listView;
}

#include "typepage.moc"
