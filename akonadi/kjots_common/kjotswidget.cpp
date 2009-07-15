/*
    This file is part of KJots.

    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "kjotswidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextBrowser>
#include <QInputDialog>

#include <QDirModel>
#include <QColumnView>

#include <QSortFilterProxyModel>

#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/control.h>
#include "collectionchildorderattribute.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodel.h>
#include "descendantentitiesproxymodel.h"
#include <akonadi/entitydisplayattribute.h>
#include "entityfilterproxymodel.h"
#include "entitytreemodel.h"
#include "entitytreeview.h"
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include "selectionproxymodel.h"

#include <grantlee/template.h>
#include <grantlee/engine.h>
#include <grantlee/context.h>

#include <KTextEdit>
#include <KLocale>
#include <KFileDialog>
#include <KMessageBox>
#include <KStandardDirs>

#include "kjotspage.h"
#include "kjotsmodel.h"

#include <kdebug.h>
#include "modeltest.h"

#include <memory>

using namespace Akonadi;
using namespace Grantlee;

KJotsWidget::KJotsWidget( QWidget * parent, Qt::WindowFlags f )
    : QWidget( parent, f )
{

  QSplitter *splitter = new QSplitter( this );
  QHBoxLayout *layout = new QHBoxLayout( this );

  KStandardDirs KStd;
  Engine *engine = Engine::instance();
  engine->setPluginDirs(KStd.findDirs("lib", "grantlee"));

  m_loader = new FileSystemTemplateLoader(this);
  m_loader->setTemplateDirs(KStd.findDirs("data", QString("kjotsrewrite/themes")));
  m_loader->setTheme("default");

  engine->addTemplateLoader(m_loader);

  treeview = new EntityTreeView( splitter );
//   treeview = new QColumnView(splitter);
//   treeview->setSortingEnabled(false);

  if ( !Akonadi::Control::start() ) {
    kFatal() << "Unable to start Akonadi server, exit application";
    return;
  }

  // TODO: Tell akonadi to load the "akonadi_kjots_resource" if it's not already loaded.
  // This can't be done synchronously, but the monitor will notify the model that it has new collections
  // when the job is done.

  // Check  Akonadi::AgentManager::self()->instances() if it contains one, if it doesn't create one and report errors
  // if necessary.

//   Akonadi::AgentManager *manager = Akonadi::AgentManager::self();
//
//   Akonadi::AgentType::List types = manager->types();
//   foreach( const Akonadi::AgentType &type, types ) {
//     qDebug() << "Type:" << type.name() << type.description() << type.identifier();
//     Akonadi::AgentInstance instance = manager->instance(type.identifier());
// // //     kDebug() << "instance:" << instance.identifier() << instance.name();
// // //     instance.setIsOnline(true);
//     instance.synchronize();
// //     Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( type );
// //     job->start();
//   }
// //   kFatal();

//   kDebug() << "ins";
//   foreach( Akonadi::AgentInstance instance, manager->instances() ) {
//     qDebug() << "Type:" << instance.name() << instance.identifier();
//     instance.setIsOnline(true);
//     instance.synchronize();
// //     Akonadi::AgentInstance instance = m
//   }
//   kDebug() << "ins done";

  // Use Collection::root as the top level 'bookshelf'
  Collection rootCollection = Collection::root();

  ItemFetchScope scope;
  scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
//   scope.fetchAttribute< CollectionChildOrderAttribute >();
  scope.fetchAttribute< EntityDisplayAttribute >();

  Monitor *monitor = new Monitor( this );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->setMimeTypeMonitored( KJotsPage::mimeType() );
//   monitor->setCollectionMonitored( rootCollection );
//   monitor->fetchCollectionStatistics( false );

  Session *session = new Session( QByteArray( "EntityTreeModel-" ) + QByteArray::number( qrand() ), this );

  etm = new KJotsModel(session, monitor, this);
//   etm->setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);
//   etm->setItemPopulationStrategy(EntityTreeModel::LazyPopulation);
//   etm->setIncludeRootCollection(true);
//   etm->setRootCollectionDisplayName("[All]");

//   DescendantEntitiesProxyModel *demp = new DescendantEntitiesProxyModel( //QModelIndex(),
//   this );
//   demp->setSourceModel(etm);
//   demp->setRootIndex(QModelIndex());

//   demp->setDisplayAncestorData(true, QString(" / "));

//   EntityFilterProxyModel *proxy = new EntityFilterProxyModel();
//   proxy->setSourceModel(etm);
//   proxy->addMimeTypeInclusionFilter( Collection::mimeType() );

//   new ModelTest(proxy, this);
//   new ModelTest( etm, this );
//   new ModelTest( dem, this );
//   new ModelTest( demp, this );

//   treeview->setModel(proxy);
//   treeview->setModel( dem );
//   treeview->setModel( demp );
//   treeview->setModel( fcp );

//   QDirModel *dm = new QDirModel(this);
//   treeview->setModel( dm );
//   treeview->setModel( proxy );
  treeview->setModel( etm );
  treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);

  selProxy = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxy->setSourceModel(etm);

  // TODO: Write a QAbstractItemView subclass to render kjots selection.
  connect(selProxy, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(renderSelection()));
  connect(selProxy, SIGNAL(rowsInserted(const QModelIndex &, int, int)), SLOT(renderSelection()));
  connect(selProxy, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), SLOT(renderSelection()));

  connect(selProxy, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)), SLOT(savePage(const QModelIndex &, int, int) ) );

  stackedWidget = new QStackedWidget( splitter );

  editor = new KTextEdit( stackedWidget );
  stackedWidget->addWidget( editor );

  layout->addWidget( splitter );

  browser = new QTextBrowser( stackedWidget );
  stackedWidget->addWidget( browser );
  stackedWidget->setCurrentWidget( browser );
}

KJotsWidget::~KJotsWidget()
{

}


void KJotsWidget::savePage(const QModelIndex &parent, int start, int end)
{
//   QModelIndexList deselectedIndexes = deselected.indexes();
//   if (deselectedIndexes.size() != 1)
//     return;
//
//   QModelIndex idx = deselectedIndexes.at(0);


  if(parent.isValid() || start != 0 || end != 0)
    return;

  const int column = 0;
  QModelIndex idx = selProxy->index(start, column, parent);
  Item item = idx.data(EntityTreeModel::ItemRole).value<Item>();
  if (!item.isValid())
    return;
  KJotsPage page = item.payload<KJotsPage>();


  kDebug() << editor->toPlainText();
  page.setContent(editor->toPlainText());
  item.setPayload(page);
  selProxy->setData(idx, QVariant::fromValue(item), EntityTreeModel::ItemRole );
}

QString KJotsWidget::renderSelectionToHtml()
{
  QHash<QString, QVariant> hash;

  QList<QVariant> objectList;

  const int rows = selProxy->rowCount();
  const int column = 0;
  for (int row = 0; row < rows; ++row)
  {
    QModelIndex idx = selProxy->index(row, column, QModelIndex());
    QObject *obj = idx.data(KJotsModel::GrantleeObjectRole).value<QObject*>();
    objectList << QVariant::fromValue(obj);
  }

  hash.insert("entities", objectList);
  Context c(hash);

  Engine *engine = Engine::instance();
  Template *t = engine->loadByName("template.html", this);

  QString result = t->render(&c);
  // TODO: handle errors.
  return result;
}

void KJotsWidget::renderSelection()
{
  const int rows = selProxy->rowCount();

  // If the selection is a single page, present it for editing...
  if (rows == 1)
  {
    QModelIndex idx = selProxy->index( 0, 0, QModelIndex());
    Item item = idx.data(EntityTreeModel::ItemRole).value<Item>();
    if (item.isValid())
    {
      KJotsPage page = item.payload<KJotsPage>();
      editor->setText( page.content() );
      stackedWidget->setCurrentWidget( editor );
      return;
    }
  }

  // ... Otherwise, render the selection read-only.

  QTextDocument doc;
  QTextCursor cursor(&doc);

  browser->setHtml( renderSelectionToHtml() );
  stackedWidget->setCurrentWidget( browser );
}

QString KJotsWidget::getThemeFromUser()
{
  bool ok;
  QString text = QInputDialog::getText(this, i18n("Change Theme"),
                                      tr("Theme name:"), QLineEdit::Normal,
                                      m_loader->themeName(), &ok);
  if (!ok || text.isEmpty())
  {
    return QString("default");
  }

  return text;

}


void KJotsWidget::changeTheme()
{
  m_loader->setTheme(getThemeFromUser());
  renderSelection();
}

void KJotsWidget::exportSelection()
{
  QString currentTheme = m_loader->themeName();
  QString themeName = getThemeFromUser();
  if (themeName.isEmpty())
  {
    themeName = "default";
  }
  m_loader->setTheme(themeName);

  QString filename = KFileDialog::getSaveFileName();
  if (!filename.isEmpty())
  {
    QFile exportFile ( filename );
    if ( !exportFile.open(QIODevice::WriteOnly | QIODevice::Text) ) {
        m_loader->setTheme(currentTheme);
        KMessageBox::error(0, i18n("<qt>Error opening internal file.</qt>"));
        return;
    }
    exportFile.write(renderSelectionToHtml().toUtf8());

    exportFile.close();
  }
  m_loader->setTheme(currentTheme);
}


#include "kjotswidget.moc"
