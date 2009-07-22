#include "couchdb-qt.h"
#include <QtCore/QCoreApplication>

int main( int argc, char** argv )
{
  QCoreApplication app( argc, argv );
  CouchDBQt c;
  c.setNotificationsEnabled( QLatin1String("contacts"), true );
  QMetaObject::invokeMethod( &c, "requestDatabaseListing", Qt::QueuedConnection );
  return app.exec();
}
