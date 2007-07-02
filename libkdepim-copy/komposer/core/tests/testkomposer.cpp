#include "core.h"

#include <kplugininfo.h>
#include <kcmdlineargs.h>
#include <kapplication.h>
#include <kdebug.h>

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, "test", 0, ki18n("test"), "0.1" , ki18n("test"));
  KApplication app;

  Komposer::Core *core = new Komposer::Core();
  app.setMainWidget( core );
  core->show();

  return app.exec();
}
