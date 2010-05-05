
#include "mainwindow.h"

#include <QHBoxLayout>
#include <QTreeView>
#include <QSplitter>
#include <QDeclarativeView>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

#include "dynamictreemodel.h"
#include "dynamictreewidget.h"

#include "breadcrumbnavigationcontext.h"
#include <qcolumnview.h>

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags f )
  : QWidget(parent, f)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  QSplitter *splitter = new QSplitter;
  layout->addWidget(splitter);

//   m_treeView = new QTreeView(splitter);
  m_treeModel = new DynamicTreeModel(this);

  DynamicTreeWidget *widget = new DynamicTreeWidget(m_treeModel, splitter);
  widget->setInitialTree(
    "- 1"
    "- - 1"
    "- - 1"
    "- - - 1"
    "- - - - 1"
    "- - - 1"
    "- - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - -1"
    "- - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - -1"
    "- - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - - 1"
    "- - - - - - 1"
    "- - - - - 1"
    "- - - - - 1"
    "- - - - 1"
    "- - - - 1"
    "- - - 1"
    "- - - 1"
    "- - - 1"
    "- 1"
    "- 1"
    "- 1"
    );

  m_declarativeView = new QDeclarativeView(splitter);

  QDeclarativeContext *context = m_declarativeView->engine()->rootContext();

//   m_treeModel = new DynamicTreeModel(this);
//   ModelInsertCommand *ins = new ModelInsertCommand( m_treeModel, this );

//   ins->setStartRow(0);
//   ins->interpret(
//     "- 1"
//     "- - 1"
//     "- - 1"
//     "- - - 1"
//     "- - - - 1"
//     "- - - 1"
//     "- - - 1"
//     "- - - - 1"
//     "- - - - 1"
//     "- - - - - 1"
//     "- - - - - 1"
//     "- - - - 1"
//     "- - - 1"
//     "- - - 1"
//     "- - - - 1"
//     "- - - - 1"
//     "- 1"
//     "- 1"
//     "- 1"
//   );
//   ins->doCommand();

//   m_treeView->setModel(m_treeModel);
//   m_treeView->expandAll();

  m_bnf = new KBreadcrumbNavigationFactory(this);
  m_bnf->setBreadcrumbDepth(2);
  m_bnf->createBreadcrumbContext( m_treeModel, this );

  widget->treeView()->setSelectionModel( m_bnf->selectionModel() );

#if 0
  QTreeView *view1 = new QTreeView;
  view1->setModel( m_bnf->selectedItemModel() );
  view1->show();
  view1->setWindowTitle( "Selected item model");
  QTreeView *view2 = new QTreeView;
  view2->setModel( m_bnf->breadcrumbItemModel() );
  view2->show();
  view2->setWindowTitle( "Breadcrumb model");
  QTreeView *view3 = new QTreeView;
  view3->setModel( m_bnf->childItemModel() );
  view3->show();
  view3->setWindowTitle( "Child items model");
#endif

  context->setContextProperty( "_selectedItemModel", QVariant::fromValue( static_cast<QObject*>( m_bnf->selectedItemModel() ) ) );
  context->setContextProperty( "_breadcrumbItemsModel", QVariant::fromValue( static_cast<QObject*>( m_bnf->breadcrumbItemModel() ) ) );
  context->setContextProperty( "_childItemsModel", QVariant::fromValue( static_cast<QObject*>( m_bnf->childItemModel() ) ) );
  context->setContextProperty( "application", QVariant::fromValue( static_cast<QObject*>( this ) ) );

  m_declarativeView->setResizeMode( QDeclarativeView::SizeRootObjectToView );
  m_declarativeView->setSource( QUrl( "./mainview.qml" ) );

  splitter->setSizes(QList<int>() << 1 << 1);

}

bool MainWindow::childCollectionHasChildren( int row )
{
  return m_bnf->childCollectionHasChildren(row);
}

void MainWindow::setSelectedChildCollectionRow( int row )
{
  m_bnf->selectChild( row );
}

void MainWindow::setSelectedBreadcrumbCollectionRow( int row )
{
  m_bnf->selectBreadcrumb( row );
}


#include "mainwindow.moc"
