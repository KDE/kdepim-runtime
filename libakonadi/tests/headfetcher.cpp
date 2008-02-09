/*
    Copyright (c) 2007 Robert Zwerus <arzie@dds.nl>

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

#include "headfetcher.h"

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QBuffer>
#include <QTimer>

#include <kcmdlineargs.h>
#include <kapplication.h>
#include <kurl.h>

using namespace Akonadi;

HeadFetcher::HeadFetcher( bool multipart )
{
  // fetch all headers from each folder
  timer.start();
  qDebug() << "Listing all headers of every folder, using" << (multipart ? "multi" : "single") << "part.";
  CollectionListJob *clj = new CollectionListJob( Collection::root() , CollectionListJob::Recursive );
  clj->exec();
  Collection::List list = clj->collections();
  foreach ( Collection collection, list ) {
    ItemFetchJob *ifj = new ItemFetchJob( collection, this );
    if ( multipart ) {
      ifj->addFetchPart( Item::PartEnvelope );
    } else {
      ifj->addFetchPart( Item::PartBody );
    }
    ifj->exec();
    qDebug() << "  Listing" << ifj->items().count() << "item headers.";
    QByteArray a;
    foreach ( Item item, ifj->items() ) {
      a = item.part( Item::PartEnvelope );
      qDebug() << a;
    }
  }

  qDebug() << "Took:" << timer.elapsed() << "ms.";
  QTimer::singleShot( 1000, this, SLOT( stop() ) );
}

void HeadFetcher::stop()
{
  qApp->quit();
}

int main( int argc, char* argv[] )
{
  KCmdLineArgs::init( argc, argv, "headfetcher", 0, ki18n("Headfetcher") ,"1.0" ,ki18n("header fetching application") );

  KCmdLineOptions options;
  options.add("multipart", ki18n("Run test on multipart data (default is singlepart)."));
  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  bool multipart = args->isSet( "multipart" );

  KApplication app( false );

  HeadFetcher d( multipart );

  return app.exec();
}

#include "headfetcher.moc"
