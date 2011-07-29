/*
    This file is part of oxaccess.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "connectiontestjob.h"

#include <kio/job.h>
#include <kurl.h>

using namespace OXA;

ConnectionTestJob::ConnectionTestJob( const QString &url, const QString &user, const QString &password, QObject *parent )
  : KJob( parent ), mUrl( url ), mUser( user ), mPassword( password )
{
}

void ConnectionTestJob::start()
{
  const KUrl url( mUrl + QString::fromLatin1( "/ajax/login?action=login&name=%1&password=%2" ).arg( mUser ).arg( mPassword ) );

  KJob *job = KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo );
  connect( job, SIGNAL(result(KJob*)), SLOT(httpJobFinished(KJob*)) );
}

void ConnectionTestJob::httpJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::StoredTransferJob *transferJob = qobject_cast<KIO::StoredTransferJob*>( job );
  Q_ASSERT( transferJob );

  const QString data = QString::fromUtf8( transferJob->data() );

  // on success data contains something like: {"session":"e530578bca504aa89738fadde9e44b3d","random":"ac9090d2cc284fed926fa3c7e316c43b"}
  // on failure data contains something like: {"category":1,"error_params":[],"error":"Invalid credentials.","error_id":"-1529642166-37","code":"LGI-0006"}
  const int index = data.indexOf( QLatin1String( "\"session\":\"" ) );
  if ( index == -1 ) { // error case
    const int errorIndex = data.indexOf( QLatin1String( "\"error\":\"" ) );
    const QString errorText = data.mid( errorIndex + 9, data.indexOf( QLatin1Char( '"' ), errorIndex + 10 ) - errorIndex - 9 );

    setError( UserDefinedError );
    setErrorText( errorText );
    emitResult();
    return;
  } else { // success case
    const QString sessionId = data.mid( index + 11, 33 ); // I assume here the session id is always 32  characters long :}

    // logout correctly...
    const KUrl url( mUrl + QString::fromLatin1( "/ajax/login?action=logout&session=%1" ).arg( sessionId ) );
    KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo );
  }

  emitResult();
}

#include "connectiontestjob.moc"
