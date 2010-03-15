/*
* This file is part of Akonadi
*
* Copyright 2010 Stephen Kelly <steveire@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include <akonadi/changerecorder.h>
#include <akonadi/entitytreeview.h>
#include <Akonadi/ItemFetchScope>
#include <akonadi/entitymimetypefiltermodel.h>

#include <kdebug.h>
#include <kselectionproxymodel.h>
#include <KStandardDirs>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
// #include <kdescendantsproxymodel.h>

#include <QBoxLayout>
#include <QSplitter>
#include <QListView>
#include <QTreeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeView>


class QmlTestWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY( int collectionRow READ selectedCollectionRow WRITE collectionRowSelected )

public:
  QmlTestWidget( QWidget *parent = 0 );

  public:
    int selectedCollectionRow() const { return 0; }
    void collectionRowSelected( int row );

};



QmlTestWidget::QmlTestWidget(QWidget* parent)
  : QWidget(parent)
{

  QHBoxLayout *mainLayout = new QHBoxLayout( this );

  Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder();
  changeRecorder->setCollectionMonitored( Akonadi::Collection::root() );

  Akonadi::EntityTreeModel *etm = new Akonadi::EntityTreeModel( changeRecorder );

  Akonadi::EntityMimeTypeFilterModel *collectionFilter = new Akonadi::EntityMimeTypeFilterModel();
  collectionFilter->setHeaderGroup( Akonadi::EntityTreeModel::CollectionTreeHeaders );
  collectionFilter->setSourceModel( etm );
  collectionFilter->addMimeTypeInclusionFilter( Akonadi::Collection::mimeType() );

#if 0
  KDescendantsProxyModel *flatProxy = new KDescendantsProxyModel( this );
  flatProxy->setSourceModel( collectionFilter );
  flatProxy->setAncestorSeparator( QLatin1String(" / ") );
  flatProxy->setDisplayAncestorData( true );
#endif

  QDeclarativeView *view = new QDeclarativeView( this );
  mainLayout->addWidget( view );
  
  view->engine()->rootContext()->setContextProperty( "collectionModel", QVariant::fromValue( static_cast<QObject*>( collectionFilter ) ) );
  view->engine()->rootContext()->setContextProperty( "application", QVariant::fromValue( static_cast<QObject*>( this ) ) );
  view->setSource( QUrl( QLatin1String("collectionviewtest.qml") ) ); // TODO make this a command line argument so this test can be used for other qml components as well
}

void QmlTestWidget::collectionRowSelected(int row)
{
  kDebug() << row;
}

int main( int argc, char **argv )
{
  const QByteArray& ba = QByteArray( "akonadi_qml" );
  const KLocalizedString name = ki18n( "Akonadi QML Test" );
  KAboutData aboutData( ba, ba, name, ba, name );
  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication app;

  QmlTestWidget testWidget;
  testWidget.show();

  return app.exec();
}

#include "qmltest.moc"
