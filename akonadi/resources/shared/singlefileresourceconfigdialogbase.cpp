/*
    Copyright (c) 2008 Bertjan Broeksema <b.broeksema@kdemail.org>
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "singlefileresourceconfigdialogbase.h"

#include <KConfigDialogManager>
#include <KFileItem>
#include <KIO/Job>
#include <KWindowSystem>

#include <QTimer>

using namespace Akonadi;

SingleFileResourceConfigDialogBase::SingleFileResourceConfigDialogBase( WId windowId ) :
    KDialog(), mStatJob( 0 ), mDirUrlChecked( false )
{
  ui.setupUi( mainWidget() );
  ui.kcfg_Path->setMode( KFile::File );
  ui.statusLabel->setVisible( false );
  setButtons( Ok | Cancel );

  if ( windowId )
    KWindowSystem::setMainWindow( this, windowId );

  connect( this, SIGNAL(okClicked()), SLOT(save()) );

  connect( ui.kcfg_Path, SIGNAL(textChanged(QString)), SLOT(validate()) );
  connect( ui.kcfg_ReadOnly, SIGNAL(toggled(bool)), SLOT(validate()) );
  connect( ui.kcfg_MonitorFile, SIGNAL(toggled(bool)), SLOT(validate()) );

  QTimer::singleShot( 0, this, SLOT(validate()) );
}

void SingleFileResourceConfigDialogBase::addPage( const QString &title, QWidget *page )
{
  ui.ktabwidget->addTab( page, title );
  mManager->updateWidgets();
}

void SingleFileResourceConfigDialogBase::setFilter(const QString & filter)
{
  ui.kcfg_Path->setFilter( filter );
}

void SingleFileResourceConfigDialogBase::validate()
{
  const KUrl currentUrl = ui.kcfg_Path->url();
  if ( currentUrl.isEmpty() ) {
    enableButton( Ok, false );
    return;
  }

  if ( currentUrl.isLocalFile() ) {
    ui.kcfg_MonitorFile->setEnabled( true );
    ui.statusLabel->setVisible( false );

    const QFileInfo file( currentUrl.path() );
    if ( file.exists() && !file.isWritable() ) {
      ui.kcfg_ReadOnly->setEnabled( false );
      ui.kcfg_ReadOnly->setChecked( true );
    } else {
      ui.kcfg_ReadOnly->setEnabled( true );
    }
    enableButton( Ok, true );
  } else {
    ui.kcfg_MonitorFile->setEnabled( false );
    ui.statusLabel->setText( i18nc( "@info:status", "Checking file information..." ) );
    ui.statusLabel->setVisible( true );

    if ( mStatJob )
      mStatJob->kill();

    mStatJob = KIO::stat( currentUrl, KIO::DefaultFlags | KIO::HideProgressInfo );
    mStatJob->setDetails( 2 ); // All details.
    mStatJob->setSide( KIO::StatJob::SourceSide );

    connect( mStatJob, SIGNAL( result( KJob * ) ),
             SLOT( slotStatJobResult( KJob * ) ) );

    // Disable the button until the MetaJob is finished.
    enableButton( Ok, false );
  }
}

void SingleFileResourceConfigDialogBase::slotStatJobResult( KJob* job )
{
  if ( job->error() == KIO::ERR_DOES_NOT_EXIST && !mDirUrlChecked ) {
    // The file did not exists, so let's see if the directory the file should
    // reside supports writing.
    const KUrl dirUrl = ui.kcfg_Path->url().upUrl();

    mStatJob = KIO::stat( dirUrl, KIO::DefaultFlags | KIO::HideProgressInfo );
    mStatJob->setDetails( 2 ); // All details.
    mStatJob->setSide( KIO::StatJob::SourceSide );

    connect( mStatJob, SIGNAL( result( KJob * ) ),
             SLOT( slotStatJobResult( KJob * ) ) );

    // Make sure we don't check the whole path upwards.
    mDirUrlChecked = true;
    return;
  } else if ( job->error() ) {
    // It doesn't seem possible to read nor write from the location so leave the
    // ok button disabled
    ui.statusLabel->setVisible( false );
    enableButton( Ok, false );
    mDirUrlChecked = false;
    mStatJob = 0;
    return;
  }

  KIO::StatJob* statJob = qobject_cast<KIO::StatJob *>( job );
  const KFileItem item( statJob->statResult(), KUrl() );

  if ( item.isWritable() ) {
    ui.kcfg_ReadOnly->setEnabled( true );
  } else {
    ui.kcfg_ReadOnly->setEnabled( false );
    ui.kcfg_ReadOnly->setChecked( true );
  }

  ui.statusLabel->setVisible( false );
  enableButton( Ok, true );

  mDirUrlChecked = false;
  mStatJob = 0;
}
