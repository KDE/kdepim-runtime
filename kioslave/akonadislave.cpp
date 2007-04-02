/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "akonadislave.h"

#include <libakonadi/itemfetchjob.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

static const KCmdLineOptions options[] =
{
  { "+protocol", I18N_NOOP( "Protocol name" ), 0 },
  { "+pool", I18N_NOOP( "Socket name" ), 0 },
  { "+app", I18N_NOOP( "Socket name" ), 0 },
  KCmdLineLastOption
};

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv) {

  KCmdLineArgs::init(argc, argv, "kio_akonadi", 0, 0, 0);
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app( false );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  AkonadiSlave slave( args->arg(1), args->arg(2) );
  slave.dispatchLoop();

  return 0;
}

using namespace Akonadi;

AkonadiSlave::AkonadiSlave(const QByteArray & pool_socket, const QByteArray & app_socket) :
    KIO::SlaveBase( "akonadi", pool_socket, app_socket )
{
  kDebug() << k_funcinfo << endl;
}

AkonadiSlave::~ AkonadiSlave()
{
  kDebug() << k_funcinfo << endl;
}

void AkonadiSlave::get(const KUrl & url)
{
  kDebug() << k_funcinfo << url << endl;
  ItemFetchJob *job = new ItemFetchJob( DataReference( url.fileName().toInt(), QString() ) );
  if ( !job->exec() ) {
    error( KIO::ERR_INTERNAL, job->errorString() );
    return;
  }
  if ( job->items().count() != 1 ) {
    error( KIO::ERR_DOES_NOT_EXIST, "No such item." );
  } else {
    const Item item = job->items().first();
    data( item.data() );
    data( QByteArray() );
    finished();
  }

  finished();
}

void AkonadiSlave::stat(const KUrl & url)
{
  kDebug() << k_funcinfo << url << endl;
  ItemFetchJob *job = new ItemFetchJob( DataReference( url.fileName().toInt(), QString() ) );
  if ( !job->exec() ) {
    error( KIO::ERR_INTERNAL, job->errorString() );
    return;
  }
  if ( job->items().count() != 1 ) {
    error( KIO::ERR_DOES_NOT_EXIST, "No such item." );
  } else {
    const Item item = job->items().first();
    KIO::UDSEntry entry;
    entry.insert( KIO::UDS_NAME, QString::number( item.reference().persistanceID() ) );
    entry.insert( KIO::UDS_MIME_TYPE,  item.mimeType() );
    statEntry( entry );
    finished();
  }
}

