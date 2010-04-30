
#include "mainwindow.h"

#include <QHBoxLayout>
#include <QTreeView>
#include <QSplitter>
#include <QDeclarativeView>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

#include "dynamictreemodel.h"

#include "breadcrumbnavigationcontext.h"
#include <qcolumnview.h>

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags f )
  : QWidget(parent, f)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  QSplitter *splitter = new QSplitter;
  layout->addWidget(splitter);

  m_treeView = new QTreeView(splitter);
  m_declarativeView = new QDeclarativeView(splitter);

  QDeclarativeContext *context = m_declarativeView->engine()->rootContext();

  m_treeModel = new DynamicTreeModel(this);
  ModelInsertCommand *ins = new ModelInsertCommand( m_treeModel, this );

  ins->setStartRow(0);
  ins->interpret(
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
    "- - - - - 1"
    "- - - - 1"
    "- - - 1"
    "- - - 1"
    "- - - - 1"
    "- - - - 1"
    "- 1"
    "- 1"
    "- 1"
  );
  ins->doCommand();

  m_treeView->setModel(m_treeModel);
  m_treeView->expandAll();

  m_bnf = new KBreadcrumbNavigationFactory(this);
  m_bnf->createBreadcrumbContext( m_treeModel, this );

  m_treeView->setSelectionModel( m_bnf->selectionModel() );

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
  if ( row < 0 )
    return false;

  //TODO Port to KProxyItemSelectionModel Or KModelIndexProxyMapper?

//    QModelIndex idx = d->mChildCollectionFilter->index( row, 0 );
#if 0
  QModelIndex idx = d->mChildCollectionFilter->index( row, 0 );
  QModelIndex idx2 = d->mChildCollectionFilter->mapToSource( idx );
  QModelIndex idx3 = d->mChildEntitiesModel->mapToSource( idx2 );
  QModelIndex idx4 = d->mSelectedSubTree->mapFromSource( idx3 );
  QModelIndex idx5 = d->mCollectionFilter->mapFromSource( idx4 );

  if ( !idx5.isValid() )
    return false;

  return idx5.model()->rowCount( idx5 ) > 0;

#endif
  return false;
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
