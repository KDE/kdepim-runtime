#include "test.h"
#include "script.h"

#include <akonadi/akonadi-xml.h>

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KDebug>

int main(int argc, char *argv[])
{
  QString path;     

  KAboutData aboutdata( "akonadi-RT", 0,
                        ki18n( "Akonadi Resource Tester" ),
                        "1.0",
                        ki18n( "Resource Tester" ),
                        KAboutData::License_GPL,
                        ki18n( "(c) 2009 Igor Trindade Oliveira" ) );

  KCmdLineArgs::init( argc, argv, &aboutdata );


  KCmdLineOptions options;
  options.add( "c" ).add( "config <configfile>", ki18n( "Configuration file to open" ), "script.js/py/rb" );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;
  
  const KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  
  if ( args->isSet( "config" ) )
    path.append(args->getOption( "config" )) ;
  else
    return -1;

  TestAgent *test = new TestAgent();
  Script *script = new Script();
  
  script->configure("/home/igor/test.js");
  script->insertObject(test, "TestResource");
  script->start();

  return app.exec();
}
