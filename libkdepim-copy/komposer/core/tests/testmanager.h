#ifndef TESTMANAGER_H
#define TESTMANAGER_H

#include <QObject>

namespace Komposer {
  class Plugin;
  class PluginManager;
}
using Komposer::Plugin;

class TestManager : public QObject
{
  Q_OBJECT
public:
  TestManager( QObject *parent );

public slots:
  void slotPluginLoaded( Plugin *plugin );
  void slotAllPluginsLoaded();
private:
  Komposer::PluginManager *m_manager;
};

#endif
