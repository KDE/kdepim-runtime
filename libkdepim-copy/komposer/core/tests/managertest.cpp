#include "managertest.h"

#include "pluginmanager.h"
using namespace Komposer;

ManagerTest::ManagerTest( QObject* parent )
    : QObject( parent )
{
    m_manager = new PluginManager( this );
}


void ManagerTest::allTests()
{
    CHECK( m_manager->availablePlugins().isEmpty(), true );
    CHECK( m_manager->loadedPlugins().empty(), true );
    CHECK( m_manager->plugin( "non-existing" ), ( Plugin* )0 );
    m_manager->shutdown();
}

#include "managertest.moc"
