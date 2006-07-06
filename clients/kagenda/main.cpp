#include "agendaview.h"
#include "dataprovider.h"

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

int main( int argc, char **argv )
{
  KAboutData aboutData( "kagenda", "KAgenda", "0.1" );
  KCmdLineArgs::init( argc, argv , &aboutData );
  KApplication app;

  AgendaView widget;

  QString arg;
  if ( argc == 2 ) arg = argv[ 1 ];

  DataProvider provider;

  if ( arg == "contact" ) provider.setupContactData();
  else if ( arg == "dummy" ) provider.setupDummyData();
  else provider.setupEventData();

  widget.setDataProvider( &provider );

  widget.show();
    
  return app.exec();
}
