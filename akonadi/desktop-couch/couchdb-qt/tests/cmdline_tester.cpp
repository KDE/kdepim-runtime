#include "couchdb-qt.h"
#include <QtCore/QCoreApplication>

int main( int argc, char** argv )
{
  QCoreApplication app( argc, argv );
  CouchDBQt c;
  QMetaObject::invokeMethod( &c, "requestDatabaseListing", Qt::QueuedConnection );
  return app.exec();
}
