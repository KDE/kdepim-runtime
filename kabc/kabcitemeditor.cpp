/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>

#include <kabc/addressee.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/monitor.h>
#include <libakonadi/session.h>

#include "kabcitemeditor.h"

class KABCItemEditor::Private
{
  public:
    Private( KABCItemEditor *parent )
      : mParent( parent ), mMonitor( 0 )
    {
    }

    ~Private()
    {
      delete mMonitor;
    }

    void fetchDone( KJob* );
    void storeDone( KJob* );
    void itemChanged( const Akonadi::Item &item, const QStringList& );

    KABCItemEditor *mParent;
    QLineEdit *mGivenName;
    QLineEdit *mFamilyName;
    Akonadi::Item mItem;
    Akonadi::Monitor *mMonitor;
};

void KABCItemEditor::Private::fetchDone( KJob *job )
{
  if ( job->error() )
    return;

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  if ( !fetchJob )
    return;

  mItem = fetchJob->items().first();

  const KABC::Addressee addr = mItem.payload<KABC::Addressee>();
  mGivenName->setText( addr.givenName() );
  mFamilyName->setText( addr.familyName() );
}

void KABCItemEditor::Private::storeDone( KJob *job )
{
}

void KABCItemEditor::Private::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  QMessageBox dlg( mParent );

  dlg.setInformativeText( QLatin1String( "The contact has been changed by anyone else\nWhat shall be done?" ) );
  dlg.addButton( QLatin1String( "Take over changes" ), QMessageBox::AcceptRole );
  dlg.addButton( QLatin1String( "Ignore and Overwrite changes" ), QMessageBox::RejectRole );

  if ( dlg.exec() == QMessageBox::AcceptRole ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( mItem.reference() );
    job->addFetchPart( Akonadi::Item::PartAll );

    mParent->connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( fetchDone( KJob* ) ) );
  }
}

KABCItemEditor::KABCItemEditor( QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  QGridLayout *layout = new QGridLayout( this );

  QLabel *label = new QLabel( QLatin1String( "Given Name:" ), this );
  layout->addWidget( label, 0, 0 );

  d->mGivenName = new QLineEdit( this );
  layout->addWidget( d->mGivenName, 0, 1 );

  label = new QLabel( QLatin1String( "Family Name:" ), this );
  layout->addWidget( label, 1, 0 );

  d->mFamilyName = new QLineEdit( this );
  layout->addWidget( d->mFamilyName, 1, 1 );
}


KABCItemEditor::~KABCItemEditor()
{
}

void KABCItemEditor::setUid( const Akonadi::DataReference &uid )
{
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( uid );
  job->addFetchPart( Akonadi::Item::PartAll );

  connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchDone( KJob* ) ) );

  delete d->mMonitor;
  d->mMonitor = new Akonadi::Monitor;
  d->mMonitor->ignoreSession( Akonadi::Session::defaultSession() );

  connect( d->mMonitor, SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
           this, SLOT( itemChanged( const Akonadi::Item&, const QStringList& ) ) );

  d->mMonitor->monitorItem( uid );
}

void KABCItemEditor::save()
{
  if ( !d->mItem.isValid() )
    return;

  KABC::Addressee addr = d->mItem.payload<KABC::Addressee>();

  addr.setGivenName( d->mGivenName->text() );
  addr.setFamilyName( d->mFamilyName->text() );
  addr.setFormattedName( QString::fromLatin1( "%1 %2" ).arg( addr.givenName() ).arg( addr.familyName() ) );

  d->mItem.setPayload<KABC::Addressee>( addr );

  Akonadi::ItemStoreJob *job = new Akonadi::ItemStoreJob( d->mItem );
  job->storePayload();
  connect( job, SIGNAL( result( KJob* ) ), SLOT( storeDone( KJob* ) ) );
}

#include "kabcitemeditor.moc"
