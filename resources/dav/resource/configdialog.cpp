/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "configdialog.h"
#include "davutils.h"
#include "settings.h"
#include "urlconfigurationdialog.h"

#include <kconfigdialogmanager.h>
#include <kconfigskeleton.h>
#include <klocale.h>

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>

ConfigDialog::ConfigDialog( QWidget *parent )
  : KDialog( parent )
{
  mUi.setupUi( mainWidget() );

  mModel = new QStandardItemModel();
  QStringList headers;
  headers << i18n( "Protocol" ) << i18n( "URL" );
  mModel->setHorizontalHeaderLabels( headers );

  mUi.configuredUrls->setModel( mModel );
  mUi.configuredUrls->setRootIsDecorated( false );

  foreach ( const QString &url, Settings::self()->remoteUrls() )
    addModelRow( DavUtils::protocolName( Settings::self()->protocol( url ) ), url );

  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();

  connect( mUi.kcfg_displayName, SIGNAL( textChanged( const QString& ) ), this, SLOT( checkUserInput() ) );
  connect( mUi.configuredUrls->selectionModel(), SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           this, SLOT( checkConfiguredUrlsButtonsState() ) );

  connect( mUi.addButton, SIGNAL( clicked() ), this, SLOT( onAddButtonClicked() ) );
  connect( mUi.removeButton, SIGNAL( clicked() ), this, SLOT( onRemoveButtonClicked() ) );
  connect( mUi.editButton, SIGNAL( clicked() ), this, SLOT( onEditButtonClicked() ) );

  connect( this, SIGNAL( okClicked() ), this, SLOT( onOkClicked() ) );
  connect( this, SIGNAL( cancelClicked() ), this, SLOT( onCancelClicked() ) );

  checkUserInput();
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::checkUserInput()
{
  checkConfiguredUrlsButtonsState();

  if ( !mUi.kcfg_displayName->text().isEmpty() && !( mModel->invisibleRootItem()->rowCount() == 0 ) )
    enableButtonOk( true );
  else
    enableButtonOk( false );
}

void ConfigDialog::onAddButtonClicked()
{
  UrlConfigurationDialog dlg( this );
  const int result = dlg.exec();

  if ( result == QDialog::Accepted ) {
    Settings::UrlConfiguration *urlConfig = Settings::self()->newUrlConfiguration( dlg.remoteUrl() );

    urlConfig->mUser = dlg.username();
    urlConfig->mProtocol = dlg.protocol();

    const QString protocolName = DavUtils::protocolName( dlg.protocol() );

    addModelRow( protocolName, dlg.remoteUrl() );
    mAddedUrls << dlg.remoteUrl();
    checkUserInput();
  }
}

void ConfigDialog::onRemoveButtonClicked()
{
  const QModelIndexList indexes = mUi.configuredUrls->selectionModel()->selectedRows();
  if ( indexes.size() == 0 )
    return;

  // There can be only one (selected row)
  const QModelIndex index = mModel->index( indexes.at( 0 ).row(), 1 );

  mRemovedUrls << index.data().toString();
  mModel->removeRow( index.row() );

  checkUserInput();
}

void ConfigDialog::onEditButtonClicked()
{
  const QModelIndexList indexes = mUi.configuredUrls->selectionModel()->selectedRows();
  if ( indexes.size() == 0 )
    return;

  // There can be only one (selected row)
  const QModelIndex index = mModel->index( indexes.at( 0 ).row(), 1 );
  const QString url = index.data().toString();

  Settings::UrlConfiguration *urlConfig = Settings::self()->urlConfiguration( url );
  if ( !urlConfig )
    return;

  UrlConfigurationDialog dlg( this );
  dlg.setRemoteUrl( urlConfig->mUrl );
  dlg.setProtocol( DavUtils::Protocol( urlConfig->mProtocol ) );
  dlg.setUsername( urlConfig->mUser );

  const int result = dlg.exec();

  if ( result == QDialog::Accepted ) {
    if ( dlg.remoteUrl() != urlConfig->mUrl ) {
      Settings::self()->removeUrlConfiguration( urlConfig->mUrl );
      urlConfig = Settings::self()->newUrlConfiguration( dlg.remoteUrl() );
    }
    urlConfig->mUser = dlg.username();
    urlConfig->mProtocol = dlg.protocol();

    QStandardItem *item = mModel->item( index.row(), 0 ); // Protocol
    item->setData( QVariant::fromValue( DavUtils::protocolName( dlg.protocol() ) ), Qt::DisplayRole );

    item = mModel->item( index.row(), 1 ); // URL
    item->setData( QVariant::fromValue( dlg.remoteUrl() ), Qt::DisplayRole );
  }
}

void ConfigDialog::onOkClicked()
{
  mManager->updateSettings();

  foreach ( const QString &url, mRemovedUrls )
    Settings::self()->removeUrlConfiguration( url );
}

void ConfigDialog::onCancelClicked()
{
  mRemovedUrls.clear();

  foreach ( const QString &url, mAddedUrls )
    Settings::self()->removeUrlConfiguration( url );
}

void ConfigDialog::checkConfiguredUrlsButtonsState()
{
  const bool enabled = mUi.configuredUrls->selectionModel()->hasSelection();

  mUi.removeButton->setEnabled( enabled );
  mUi.editButton->setEnabled( enabled );
}

void ConfigDialog::addModelRow( const QString &protocol, const QString &url )
{
  QStandardItem *rootItem = mModel->invisibleRootItem();
  QList<QStandardItem*> items;

  QStandardItem *protocolStandardItem = new QStandardItem( protocol );
  protocolStandardItem->setEditable( false );
  items << protocolStandardItem;

  QStandardItem *urlStandardItem = new QStandardItem( url );
  urlStandardItem->setEditable( false );
  items << urlStandardItem;

  rootItem->appendRow( items );
}

#include "configdialog.moc"
