#ifndef MANAGERTEST_H
#define MANAGERTEST_H

#include <qobject.h>
#include "tester.h"

namespace Komposer {
    class PluginManager;
}

class ManagerTest : public QObject,
                    public Tester
{
    Q_OBJECT
public:
    ManagerTest( QObject* parent = 0 );

public slots:
    void allTests();
private:
    Komposer::PluginManager* m_manager;
};

#endif
