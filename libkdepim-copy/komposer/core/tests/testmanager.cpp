#include "testmanager.h"
#include "testmanager.moc"

#include "pluginmanager.h"
#include "plugin.h"

#include <kplugininfo.h>
#include <kcmdlineargs.h>
#include <kapplication.h>
#include <kdebug.h>

using namespace Komposer;

TestManager::TestManager( QObject *parent )
  : QObject( parent )
{
  m_manager = new PluginManager( this );
  connect( m_manager, SIGNAL(pluginLoaded(Plugin*)),
           SLOT(slotPluginLoaded(Plugin*)) );
  connect( m_manager, SIGNAL(allPluginsLoaded()),
           SLOT(slotAllPluginsLoaded()) );
  m_manager->loadAllPlugins();

  QValueList<KPluginInfo*> plugins = m_manager->availablePlugins();
  kDebug()<<"Number of available plugins is "<< plugins.count() <<endl;
  for ( QValueList<KPluginInfo*>::iterator it = plugins.begin(); it != plugins.end(); ++it ) {
    KPluginInfo *i = ( *it );
    kDebug()<<"\tAvailable plugin "<< i->name()
             <<", comment = "<< i->comment() <<endl;
  }
}

void TestManager::slotAllPluginsLoaded()
{
  kDebug()<<"Done"<<endl;
  m_manager->shutdown();
  qApp->exit();
}

void TestManager::slotPluginLoaded( Plugin *plugin )
{
  kDebug()<<"A plugin "<< m_manager->pluginName( plugin ) << " has been loaded"<<endl;
}

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, "test", 0, ki18n("test"), "0.1" , ki18n("test"));
  KApplication app;

  TestManager manager( &app );

  return app.exec();
}
