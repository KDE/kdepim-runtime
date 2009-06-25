/*
 * Copyright 2009 Kevin Ottens <ervin@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kxmlguiwindow.h>

#include <QSplitter>
#include <QTreeView>

#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include "../entitytreemodel.h"
#include "../statisticstooltipproxymodel.h"

using namespace Akonadi;

class MainWindow : public KXmlGuiWindow
{
  Q_OBJECT
public:
  MainWindow()
  {
    QSplitter *splitter = new QSplitter( this );

    QTreeView *treeView1 = new QTreeView( splitter );
    QTreeView *treeView2 = new QTreeView( splitter );

    Session *session = new Session( "Statistics Proxy test app", this );

    // monitor collection changes
    Monitor *monitor = new Monitor( this );
    monitor->setCollectionMonitored( Collection::root() );
    monitor->fetchCollection( true );
    monitor->setAllMonitored( true );

    EntityTreeModel *baseModel = new EntityTreeModel( session, monitor, this );
    baseModel->setIncludeRootCollection( true );
    treeView2->setModel( baseModel );

    StatisticsToolTipProxyModel *proxy1 = new StatisticsToolTipProxyModel( this );
    proxy1->setSourceModel( baseModel );
    treeView1->setModel( proxy1 );

    setCentralWidget( splitter );
  }

  ~MainWindow()
  {

  }
};

int main( int argc, char **argv )
{
  const QByteArray& ba = QByteArray( "statisticsmodel_testapp" );
  const KLocalizedString name = ki18n( "Statistics Proxy models test app" );
  KAboutData aboutData( ba, ba, name, ba, name );
  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication app;
  MainWindow* mw = new MainWindow();
  mw->show();
  app.exec();
}

#include "statisticsmodeltestapp.moc"
