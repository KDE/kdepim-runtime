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
#include <akonadi/changerecorder.h>
#include <akonadi/control.h>
#include "collectionchildorderattribute.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodel.h>
#include <kdescendantsproxymodel.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entityfilterproxymodel.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/entitytreeview.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <kselectionproxymodel.h>
// #include <akonadi/partfetcher.h>

#include <grantlee/template.h>
#include <grantlee/engine.h>
#include <grantlee/context.h>

#include <KTextEdit>
#include <KLocale>
#include <KFileDialog>
#include <KMessageBox>
#include <KStandardDirs>
#include <KMime/KMimeMessage>

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
  engine->setPluginDirs(KStd.findDirs("lib", QLatin1String( "grantlee" ) ) );

  m_loader = FileSystemTemplateLoader::Ptr( new FileSystemTemplateLoader() );
  m_loader->setTemplateDirs(KStd.findDirs("data", QLatin1String("kjotsrewrite/themes")));
  m_loader->setTheme( QLatin1String( "default" ) );

  engine->addTemplateLoader( m_loader );

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
//   scope.fetchPayloadPart( "title" );
  scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
//   scope.fetchAttribute< CollectionChildOrderAttribute >();
  scope.fetchAttribute< EntityDisplayAttribute >();

  ChangeRecorder *monitor = new ChangeRecorder( this );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->setMimeTypeMonitored( QLatin1String( "text/x-vnd.akonadi.note" ) );
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

  selProxy = new KSelectionProxyModel(treeview->selectionModel(), this);
  selProxy->setSourceModel(etm);

  // TODO: Write a QAbstractItemView subclass to render kjots selection.
  connect(selProxy, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(renderSelection()));
  connect(selProxy, SIGNAL(rowsInserted(const QModelIndex &, int, int)), SLOT(renderSelection()));
  connect(selProxy, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), SLOT(renderSelection()));

  stackedWidget = new QStackedWidget( splitter );

  editor = new KTextEdit( stackedWidget );
  stackedWidget->addWidget( editor );

  layout->addWidget( splitter );

  browser = new QTextBrowser( stackedWidget );
  stackedWidget->addWidget( browser );
  stackedWidget->setCurrentWidget( browser );
}

void KJotsWidget::itemActivated(const QModelIndex& index )
{
  return;
}

void KJotsWidget::partFetched(const QModelIndex& index, Item item, const QByteArray& partName)
{
  disconnect (sender(), SIGNAL(partFetched(QModelIndex,Item,QByteArray)), this, SLOT(partFetched(QModelIndex,Item,QByteArray)));
  disconnect (sender(), SIGNAL(invalidated()), this, SLOT(invalidated()));

  delete sender();
}

void KJotsWidget::invalidated()
{
  kDebug() << sender() << "invalidated";
}



KJotsWidget::~KJotsWidget()
{

}


void KJotsWidget::savePage(const QModelIndex &parent, int start, int end)
{
  // Disable this for now.
  return;

  if(parent.isValid() || start != 0 || end != 0)
    return;

  const int column = 0;
  QModelIndex idx = selProxy->index(start, column, parent);
  Item item = idx.data(EntityTreeModel::ItemRole).value<Item>();
  if (!item.isValid())
    return;

  if (!item.hasPayload<KMime::Message::Ptr>())
    return;

  KMime::Message::Ptr page = item.payload<KMime::Message::Ptr>();



//   page.setContent(editor->toPlainText());
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

//     PartFetcher *fetcher = new PartFetcher(this);
//     connect (fetcher, SIGNAL(partFetched(QModelIndex,Item,QByteArray)), SLOT(partFetched(QModelIndex,Item,QByteArray)));
//     connect (fetcher, SIGNAL(invalidated()), SLOT(invalidated()));
//     if (!fetcher->fetchPart(idx, "content"))
//     {
//       disconnect (fetcher, SIGNAL(partFetched(QModelIndex,Item,QByteArray)), this, SLOT(partFetched(QModelIndex,Item,QByteArray)));
//       disconnect (fetcher, SIGNAL(invalidated()), this, SLOT(invalidated()));
//     }

    QObject *obj = idx.data(KJotsModel::GrantleeObjectRole).value<QObject*>();
    objectList << QVariant::fromValue(obj);
  }

  hash.insert( QLatin1String( "entities" ), objectList);
  Context c(hash);

  Engine *engine = Engine::instance();
  Template t = engine->loadByName( QLatin1String( "template.html" ) );

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

//     PartFetcher *fetcher = new PartFetcher(this);
//     connect (fetcher, SIGNAL(partFetched(QModelIndex,Item,QByteArray)), SLOT(partFetched(QModelIndex,Item,QByteArray)));
//     connect (fetcher, SIGNAL(invalidated()), SLOT(invalidated()));
//     if (!fetcher->fetchPart(idx, "content"))
//     {
//       disconnect (fetcher, SIGNAL(partFetched(QModelIndex,Item,QByteArray)), this, SLOT(partFetched(QModelIndex,Item,QByteArray)));
//       disconnect (fetcher, SIGNAL(invalidated()), this, SLOT(invalidated()));
//     }

    Item item = idx.data(EntityTreeModel::ItemRole).value<Item>();
    if (item.isValid())
    {
      if (!item.hasPayload<KMime::Message::Ptr>())
        return;

      KMime::Message::Ptr page = item.payload<KMime::Message::Ptr>();
      editor->setText( page->mainBodyPart()->decodedText() );
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
    return QLatin1String("default");
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
    themeName = QLatin1String( "default" );
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
