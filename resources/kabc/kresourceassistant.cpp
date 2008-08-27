/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "kresourceassistant.h"

#include <kresources/configwidget.h>
#include <kresources/factory.h>
#include <kresources/resource.h>

#include <kdebug.h>
#include <klineedit.h>
#include <kpagewidget.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QStackedWidget>

class KResourceDescriptionLabel : public QWidget
{
  public:
    KResourceDescriptionLabel( const QString &type, const QString &desc, QWidget *parent)
      : QWidget( parent ), mType( type )
    {
      QVBoxLayout *mainLayout = new QVBoxLayout( this );

      QLabel *label = new QLabel( desc, this );
      label->setWordWrap( true );
      mainLayout->addWidget( label );

      mainLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding ) );
    }

  public:
    const QString mType;
};

class KResourceCreationWidget : public QWidget
{
  public:
    KResourceCreationWidget( KRES::Factory *factory, QWidget *parent )
      : QWidget( parent ), mFactory( factory ), mResource( 0 ), mPageWidget( 0 )
    {
      QVBoxLayout *mainLayout = new QVBoxLayout( this );
      mainLayout->setSpacing( KDialog::spacingHint() );
      mainLayout->setMargin( KDialog::marginHint() );

      mPageWidget = new KPageWidget( this );
      mPageWidget->setFaceType( KPageView::Tree );

      mainLayout->addWidget( mPageWidget );

      mTypes = mFactory->typeNames();
      int index = mTypes.indexOf( QLatin1String( "akonadi" ) );
      if ( index != -1 )
        mTypes.removeAt( index );

      foreach ( const QString &type, mTypes ) {
        QString description = mFactory->typeDescription( type );
        if ( description.isEmpty() )
          description = i18nc( "@info", "No description available" );

        QWidget *label = new KResourceDescriptionLabel( type, description, mPageWidget );
        mPageWidget->addPage( label, mFactory->typeName( type ) );
      }
    }

    void createResource()
    {
      KResourceDescriptionLabel *label =
        static_cast<KResourceDescriptionLabel*>( mPageWidget->currentPage()->widget() );

      if ( mResource != 0 && mResource->type() == label->mType )
        return;

      delete mResource;
      mResource = mFactory->resource( label->mType );
    }

  public:
    QStringList mTypes;

    KRES::Factory  *mFactory;
    KRES::Resource *mResource;

    KPageWidget *mPageWidget;
};

class KResourcePluginConfigWidget : public QGroupBox
{
  public:
    KResourcePluginConfigWidget( const QString &type, KRES::Factory *factory, QWidget *parent)
      : QGroupBox( parent ), mPluginWidget( 0 )
    {
      setTitle( i18nc( "@title:group", "%1 Plugin Settings", factory->typeName( type ) ) );

      QVBoxLayout *mainLayout = new QVBoxLayout( this );
      mPluginWidget = factory->configWidget( type, this );
      if ( mPluginWidget == 0 ) {
        kError() << "No plugin configuration widget for resource type" << type;
        QLabel *label = new QLabel( i18nc( "@info", "No plugin specific configuration available" ), this );
        label->setAlignment( Qt::AlignHCenter );
        mainLayout->addWidget( label );
      } else
        mainLayout->addWidget( mPluginWidget );

      mainLayout->addStretch();
    }

    public:
      KRES::ConfigWidget *mPluginWidget;
};

class KResourceConfigWidget : public QWidget
{
  public:
    KResourceConfigWidget( const QStringList &types, KRES::Factory *factory, QWidget *parent )
      : QWidget( parent ), mName( 0 ), mReadOnly( 0 ), mStackWidget( 0 )
    {
      QVBoxLayout *mainLayout = new QVBoxLayout( this );
      mainLayout->setSpacing( KDialog::spacingHint() );
      mainLayout->setMargin( KDialog::marginHint() );

      QGroupBox *generalGroup = new QGroupBox( this );
      QGridLayout *generalLayout = new QGridLayout( generalGroup );
      generalLayout->setSpacing( KDialog::spacingHint() );

      generalGroup->setTitle( i18nc( "@title:group", "General Settings" ) );

      QLabel *nameLabel = new QLabel( i18nc( "@label resource name", "Name:" ), generalGroup );
      generalLayout->addWidget( nameLabel, 0, 0 );

      mName = new KLineEdit( generalGroup );
      generalLayout->addWidget( mName, 0, 1 );

      connect( mName, SIGNAL( textChanged( const QString& ) ),
               parent, SLOT( slotNameChanged( const QString& ) ) );

      mReadOnly = new QCheckBox( i18nc( "@option:check if resource is read-only",
                                         "Read-only" ),
                                 generalGroup );
      generalLayout->addWidget( mReadOnly, 1, 0, 1, 2 );

      mReadOnly->setChecked( false );

      mainLayout->addWidget( generalGroup );

      mStackWidget = new QStackedWidget( this );

      mainLayout->addWidget( mStackWidget );

      foreach ( const QString &type, types ) {
        KResourcePluginConfigWidget *configWidget =
          new KResourcePluginConfigWidget( type, factory, mStackWidget );
        mStackWidget->addWidget( configWidget );
        mStackedWidgets.insert( type, configWidget );

        connect( configWidget->mPluginWidget, SIGNAL( setReadOnly( bool ) ),
                 parent, SLOT( setReadOnly( bool ) ) );
      }
    }

    void loadSettings( KRES::Resource *resource )
    {
      KResourcePluginConfigWidget *widget = mStackedWidgets[ resource->type() ];
      Q_ASSERT( widget != 0 );

      if ( widget->mPluginWidget != 0 )
        widget->mPluginWidget->loadSettings( resource );
      mStackWidget->setCurrentWidget( widget );
    }

    void saveSettings( KRES::Resource *resource )
    {
      KResourcePluginConfigWidget *widget = mStackedWidgets[ resource->type() ];
      Q_ASSERT( widget != 0 );

      if ( widget->mPluginWidget != 0 )
        widget->mPluginWidget->saveSettings( resource );
    }

  public:
    KLineEdit *mName;
    QCheckBox *mReadOnly;

    QStackedWidget *mStackWidget;
    QMap<QString, KResourcePluginConfigWidget*> mStackedWidgets;
};

class KResourceAssistant::Private
{
  public:
    explicit Private( KResourceAssistant *parent )
      : mParent( parent ), mFactory( 0 ), mCreationWidget( 0 ),
        mConfigWidget( 0 )
    {
    }

  public:
    KResourceAssistant *mParent;

    KRES::Factory *mFactory;

    KResourceCreationWidget *mCreationWidget;
    KResourceConfigWidget   *mConfigWidget;
};

KResourceAssistant::KResourceAssistant( const QString& resourceFamily, QWidget *parent )
  : KAssistantDialog( parent ), d( new Private( this ) )
{
  setModal( true );
  setCaption( i18nc( "@title:window", "KDE Compatibility Assistant" ) );

  d->mFactory = KRES::Factory::self( resourceFamily.toLower() );

  d->mCreationWidget = new KResourceCreationWidget( d->mFactory, this );
  addPage( d->mCreationWidget, i18nc( "@title:tab", "Plugin Selection") );

  d->mConfigWidget =
    new KResourceConfigWidget( d->mCreationWidget->mTypes, d->mFactory, this );
  addPage( d->mConfigWidget, i18nc( "@title:tab", "Plugin Configuration") );
}

KResourceAssistant::~KResourceAssistant()
{
}

KRES::Resource *KResourceAssistant::resource()
{
  return d->mCreationWidget->mResource;
}

void KResourceAssistant::back()
{
  KPageWidgetItem *item = currentPage();
  if ( item->widget() == d->mConfigWidget ) {
    d->mConfigWidget->saveSettings( d->mCreationWidget->mResource );
  }

  KAssistantDialog::back();
}

void KResourceAssistant::next()
{
  KPageWidgetItem *item = currentPage();
  if ( item->widget() == d->mCreationWidget ) {
    d->mCreationWidget->createResource();

    d->mConfigWidget->loadSettings( d->mCreationWidget->mResource );
  }

  KAssistantDialog::next();
}

#include "kresourceassistant.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
