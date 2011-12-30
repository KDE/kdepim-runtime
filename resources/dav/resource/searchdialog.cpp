/*
 *  Copyright (c) 2011 Grégory Oestreicher <greg@kamago.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "searchdialog.h"

#include "davcollectionsfetchjob.h"
#include "davmanager.h"
#include "davprincipalsearchjob.h"
#include "davprotocolbase.h"
#include "davutils.h"

#include <KDebug>
#include <KIcon>
#include <KMessageBox>
#include <KUrl>

#include <QtGui/QStandardItemModel>

SearchDialog::SearchDialog( QWidget *parent )
  : KDialog( parent ), mModel( new QStandardItemModel( this ) ), mSubJobCount( 0 )
{
  mUi.setupUi( mainWidget() );
  mUi.credentialsGroup->setVisible( false );
  mUi.searchResults->setModel( mModel );

  setButtonFocus( KDialog::None );
  setDefaultButton( KDialog::None );
  setButtonText( KDialog::Ok, i18n( "Add selected items" ) );

  connect( mUi.searchUrl, SIGNAL(textChanged(const QString&)), this, SLOT(checkUserInput()) );
  connect( mUi.searchParam, SIGNAL(textChanged(const QString&)), this, SLOT(checkUserInput()) );
  connect( mUi.searchButton, SIGNAL(clicked()), this, SLOT(search()) );

  checkUserInput();
}

SearchDialog::~SearchDialog()
{
}

bool SearchDialog::useDefaultCredentials() const
{
  return mUi.useDefaultCreds->isChecked();
}

void SearchDialog::setUsername( const QString& user )
{
  mUi.username->setText( user );
}

QString SearchDialog::username() const
{
  return mUi.username->text();
}

void SearchDialog::setPassword( const QString& password )
{
  mUi.password->setText( password );
}

QString SearchDialog::password() const
{
  return mUi.password->text();
}

QStringList SearchDialog::selection() const
{
  QModelIndexList indexes = mUi.searchResults->selectionModel()->selectedIndexes();
  QStringList ret;
  foreach ( const QModelIndex &index, indexes ) {
    kError() << "SELECTED DATA: " << index.data( Qt::UserRole + 1 ).toString();
    ret << index.data( Qt::UserRole + 1 ).toString();
  }
  return ret;
}

void SearchDialog::checkUserInput()
{
  if ( mUi.searchUrl->text().isEmpty() || mUi.searchParam->text().isEmpty() )
    mUi.searchButton->setEnabled( false );
  else
    mUi.searchButton->setEnabled( true );
}

void SearchDialog::search()
{
  mUi.searchResults->setEnabled( false );
  mModel->clear();
  DavPrincipalSearchJob::FilterType filter;

  if ( mUi.searchType->currentIndex() == 0 )
    filter = DavPrincipalSearchJob::DisplayName;
  else
    filter = DavPrincipalSearchJob::EmailAddress;

  KUrl url( mUi.searchUrl->text() );
  url.setUser( mUi.username->text() );
  url.setPassword( mUi.password->text() );
  DavUtils::DavUrl davUrl;
  davUrl.setUrl( url );

  DavPrincipalSearchJob *job = new DavPrincipalSearchJob( davUrl, filter, mUi.searchParam->text() );

  const DavProtocolBase *proto = DavManager::self()->davProtocol( DavUtils::CalDav );
  job->fetchProperty( proto->principalHomeSet(), proto->principalHomeSetNS() );

  proto = DavManager::self()->davProtocol( DavUtils::CardDav );
  job->fetchProperty( proto->principalHomeSet(), proto->principalHomeSetNS() );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(onSearchJobFinished(KJob*)) );
  job->start();
}

void SearchDialog::onSearchJobFinished( KJob* job )
{
  if ( job->error() ) {
    KMessageBox::error( this, i18n( "An error occurred when executing search:\n%1", job->errorText() ) );
    return;
  }

  DavPrincipalSearchJob *davJob = qobject_cast<DavPrincipalSearchJob*>( job );
  QList<DavPrincipalSearchJob::Result> results = davJob->results();

  const DavProtocolBase *caldav = DavManager::self()->davProtocol( DavUtils::CalDav );
  DavUtils::DavUrl davUrl = davJob->davUrl();
  KUrl url = davUrl.url();

  foreach ( const DavPrincipalSearchJob::Result &result, results ) {
    if ( result.value.startsWith( '/' ) ) {
      url.setPath( result.value );
    }
    else {
      KUrl tmp( result.value );
      tmp.setUser( url.user() );
      tmp.setPassword( url.password() );
      url = tmp;
    }
    davUrl.setUrl( url );

    if ( result.property == caldav->principalHomeSet() )
      davUrl.setProtocol( DavUtils::CalDav );
    else
      davUrl.setProtocol( DavUtils::CardDav );

    DavCollectionsFetchJob *fetchJob = new DavCollectionsFetchJob( davUrl );
    connect( fetchJob, SIGNAL(result(KJob*)), this, SLOT(onCollectionsFetchJobFinished(KJob*)) );
    fetchJob->start();
    ++mSubJobCount;
  }
}

void SearchDialog::onCollectionsFetchJobFinished( KJob* job )
{
  --mSubJobCount;

  if ( mSubJobCount == 0 )
    mUi.searchResults->setEnabled( true );

  if ( job->error() ) {
    if ( mSubJobCount == 0 )
      KMessageBox::error( this, i18n( "An error occurred when executing search:\n%1", job->errorText() ) );
    return;
  }

  DavCollectionsFetchJob *davJob = qobject_cast<DavCollectionsFetchJob*>( job );
  DavCollection::List collections = davJob->collections();

  foreach ( const DavCollection &collection, collections ) {
    QStandardItem *item = new QStandardItem( collection.displayName() );
    QString data( DavUtils::protocolName( collection.protocol() ) + '|' + collection.url() );
    item->setData( data, Qt::UserRole + 1 );
    item->setToolTip( collection.url() );
    if ( collection.protocol() == DavUtils::CalDav )
      item->setIcon( KIcon( "view-calendar" ) );
    else
      item->setIcon( KIcon( "view-pim-contacts" ) );
    mModel->appendRow( item );
  }
}
