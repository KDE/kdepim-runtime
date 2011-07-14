/*
    Copyright (c) 2011 Tobias Koenig <tokoe@kde.org>

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

#include "freebusyupdatehandler.h"

#include <kdebug.h>
#include <kio/job.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kurl.h>

#include <QtCore/QTimer>

FreeBusyUpdateHandler::FreeBusyUpdateHandler( QObject *parent )
  : QObject( parent ), mTimer( new QTimer( this ) )
{
  mTimer->setInterval( 2000 );
  connect( mTimer, SIGNAL( timeout() ), SLOT( timeout() ) );
}

FreeBusyUpdateHandler::~FreeBusyUpdateHandler()
{
  mTimer->stop();
}

void FreeBusyUpdateHandler::updateFolder( const QString &folderPath, const QString &userName, const QString &password, const QString &host )
{
  QString path( folderPath );

  /* Steffen said: you must issue an authenticated HTTP GET request to
     https://kolabserver/freebusy/trigger/user@domain/Folder/NestedFolder.pfb
     (replace .pfb with .xpfb for extended fb lists). */

  KUrl httpUrl;
  httpUrl.setUser( userName );
  httpUrl.setPassword( password );
  httpUrl.setHost( host );
  httpUrl.setProtocol( QLatin1String( "https" ) );

  // IMAP path is either /INBOX/<path> or /user/someone/<path>
  Q_ASSERT( path.startsWith( "/" ) );
  const int secondSlash = path.indexOf( '/', 1 );
  if ( secondSlash == -1 ) {
    kWarning() << "path is too short: " << path;
    return;
  }

  if ( path.startsWith( "/INBOX/", Qt::CaseInsensitive ) ) {
    // If INBOX, replace it with the username (which is user@domain)
    path = path.mid( secondSlash );
    path.prepend( userName );
  } else {
    // If user, just remove it. So we keep the IMAP-returned username.
    // This assumes it's a known user on the same domain.
    path = path.mid( secondSlash );
  }

  if ( path.startsWith( "/" ) )
    httpUrl.setPath( "/freebusy/trigger" + path + ".pfb" );
  else
    httpUrl.setPath( "/freebusy/trigger/" + path + ".pfb" );

  mUrls.insert( httpUrl );
  mTimer->start();
}

void FreeBusyUpdateHandler::timeout()
{
  foreach ( const KUrl &url, mUrls ) {
    kDebug() << "Triggering PFB update for " << url;

    KIO::Job* job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    job->addMetaData( QLatin1String( "errorPage" ), QLatin1String( "false" ) ); // we want an error in case of 404
    connect( job, SIGNAL( result( KJob* ) ), SLOT( slotFreeBusyTriggerResult( KJob* ) ) );
  }

  mUrls.clear();
}

void FreeBusyUpdateHandler::slotFreeBusyTriggerResult( KJob *job )
{
  if ( job->error() ) {
    KMessageBox::sorry( 0, i18n( "Could not trigger Free/Busy information update: %1." ).arg( job->errorText() ) );
  }
}

