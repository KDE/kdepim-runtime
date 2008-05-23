/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kcmdesignerfields.h"

#include <KAboutData>
#include <KDebug>
#include <KDialog>
#include <KDirWatch>
#include <KFileDialog>
#include <KGlobal>
#include <KLocale>
#include <KMessageBox>
#include <KRun>
#include <KShell>
#include <KStandardDirs>
#include <K3ListView>
#include <kio/job.h>
#include <kio/netaccess.h>

#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QRegExp>
#include <QTimer>
#include <QtDesigner/QFormBuilder>
#include <QHBoxLayout>
#include <QFrame>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <Q3GroupBox>

#include <unistd.h>

using namespace KPIM;

namespace KPIM {

class PageItem : public Q3CheckListItem
{
  public:
    PageItem( Q3ListView *parent, const QString &path )
      : Q3CheckListItem( parent, "", Q3CheckListItem::CheckBox ),
        mPath( path ), mIsActive( false )
    {
      mName = path.mid( path.lastIndexOf( '/' ) + 1 );

      QFile f( mPath );
      QFormBuilder builder;
      QWidget *wdg = builder.load( &f, 0 );
      if ( wdg ) {
        setText( 0, wdg->windowTitle() );

        QPixmap pm = QPixmap::grabWidget( wdg );
        QImage img = pm.toImage().scaled( 300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        mPreview = QPixmap::fromImage(img);

        QList<QWidget*> list = wdg->findChildren<QWidget*>();
        QWidget *it;

        QMap<QString, QString> allowedTypes;
        allowedTypes.insert( "QLineEdit", i18n( "Text" ) );
        allowedTypes.insert( "QTextEdit", i18n( "Text" ) );
        allowedTypes.insert( "QSpinBox", i18n( "Numeric Value" ) );
        allowedTypes.insert( "QCheckBox", i18n( "Boolean" ) );
        allowedTypes.insert( "QComboBox", i18n( "Selection" ) );
        allowedTypes.insert( "QDateTimeEdit", i18n( "Date & Time" ) );
        allowedTypes.insert( "KLineEdit", i18n( "Text" ) );
        allowedTypes.insert( "KDateTimeWidget", i18n( "Date & Time" ) );
        allowedTypes.insert( "KDatePicker", i18n( "Date" ) );

        Q_FOREACH( it, list ) {
          if ( allowedTypes.contains( it->metaObject()->className() )  ) {
            QString name = it->objectName();
            if ( name.startsWith( "X_" ) ) {
              new Q3ListViewItem( this, name,
                                 allowedTypes[ it->metaObject()->className() ],
                                 it->metaObject()->className(),
                                 it->whatsThis() );
            }
          }
        }

      }
    }

    QString name() const { return mName; }
    QString path() const { return mPath; }

    QPixmap preview()
    {
      return mPreview;
    }

    void setIsActive( bool isActive ) { mIsActive = isActive; }
    bool isActive() const { return mIsActive; }

  protected:
    void paintBranches( QPainter *p, const QColorGroup & cg, int w, int y, int h )
    {
      Q3ListViewItem::paintBranches( p, cg, w, y, h );
    }

  private:
    QString mName;
    QString mPath;
    QPixmap mPreview;
    bool mIsActive;
};

}


KCMDesignerFields::KCMDesignerFields( const KComponentData &instance, QWidget *parent, const QStringList &args )
  : KCModule( instance, parent, args )
   ,mPageView(NULL)
   ,mPagePreview(NULL)
   ,mPageDetails(NULL)
   ,mDeleteButton(NULL)
   ,mImportButton(NULL)
   ,mDesignerButton(NULL)
{
  KAboutData *about = new KAboutData( I18N_NOOP( "KCMDesignerfields" ), 0,
                                      ki18n( "Qt Designer Fields Dialog" ),
                                      0, KLocalizedString(), KAboutData::License_LGPL,
                                      ki18n( "(c), 2004 Tobias Koenig" ) );

  about->addAuthor( ki18n("Tobias Koenig"), KLocalizedString(), "tokoe@kde.org" );
  about->addAuthor( ki18n("Cornelius Schumacher"), KLocalizedString(), "schumacher@kde.org" );
  setAboutData( about );
}

void KCMDesignerFields::delayedInit()
{
  kDebug() <<"KCMDesignerFields::delayedInit()";

  initGUI();

  connect( mPageView, SIGNAL( selectionChanged( Q3ListViewItem* ) ),
           this, SLOT( updatePreview( Q3ListViewItem* ) ) );
  connect( mPageView, SIGNAL( clicked( Q3ListViewItem* ) ),
           this, SLOT( itemClicked( Q3ListViewItem* ) ) );

  connect( mDeleteButton, SIGNAL( clicked() ),
           this, SLOT( deleteFile() ) );
  connect( mImportButton, SIGNAL( clicked() ),
           this, SLOT( importFile() ) );
  connect( mDesignerButton, SIGNAL( clicked() ),
           this, SLOT( startDesigner() ) );

  load();

  // Install a dirwatcher that will detect newly created or removed designer files
  KDirWatch *dw = new KDirWatch( this );
  dw->addDir( localUiDir(), KDirWatch::WatchFiles );
  connect( dw, SIGNAL( created(const QString&) ), SLOT( rebuildList() ) );
  connect( dw, SIGNAL( deleted(const QString&) ), SLOT( rebuildList() ) );
  connect( dw, SIGNAL( dirty(const QString&) ),   SLOT( rebuildList() ) );
}

void KCMDesignerFields::deleteFile()
{
  Q3ListViewItem *item = mPageView->selectedItem();
  if ( item ) {
    PageItem *pageItem = static_cast<PageItem*>( item->parent() ? item->parent() : item );
    if (KMessageBox::warningContinueCancel(this,
	i18n( "<qt>Do you really want to delete '<b>%1</b>'?</qt>", pageItem->text(0) ), "", KStandardGuiItem::del() )
         == KMessageBox::Continue)
      KIO::NetAccess::del( pageItem->path(), 0 );
  }
  // The actual view refresh will be done automagically by the slots connected to kdirwatch
}

void KCMDesignerFields::importFile()
{
  KUrl src = KFileDialog::getOpenFileName( QDir::homePath(), i18n("*.ui|Designer Files"),
                                              this, i18n("Import Page") );
  KUrl dest = localUiDir();
  dest.setFileName(src.fileName());
  KIO::Job *job = KIO::file_copy( src, dest, -1, KIO::Overwrite );
  KIO::NetAccess::synchronousRun( job, this );

  // The actual view refresh will be done automagically by the slots connected to kdirwatch
}


void KCMDesignerFields::loadUiFiles()
{
  QStringList list = KGlobal::dirs()->findAllResources( "data", uiPath() + "/*.ui",
                                                        KStandardDirs::Recursive |
                                                        KStandardDirs::NoDuplicates );
  for ( QStringList::iterator it = list.begin(); it != list.end(); ++it ) {
    new PageItem( mPageView, *it );
  }
}

void KCMDesignerFields::rebuildList()
{
  // If nothing is initialized there is no need to do something
  if (mPageView) {
    QStringList ai = saveActivePages();
    updatePreview( 0 );
    mPageView->clear();
    loadUiFiles();
    loadActivePages(ai);
  }
}

void KCMDesignerFields::loadActivePages(const QStringList& ai)
{
  Q3ListViewItemIterator it( mPageView );
  while ( it.current() ) {
    if ( it.current()->parent() == 0 ) {
      PageItem *item = static_cast<PageItem*>( it.current() );
      if ( ai.contains( item->name() )  ) {
        item->setOn( true );
        item->setIsActive( true );
      }
    }

    ++it;
  }
}

void KCMDesignerFields::load()
{
  // see KCModule::showEvent()
  if (!mPageView) {
    delayedInit();
  }
  loadActivePages( readActivePages() );
}

QStringList KCMDesignerFields::saveActivePages()
{
  Q3ListViewItemIterator it( mPageView, Q3ListViewItemIterator::Checked |
                            Q3ListViewItemIterator::Selectable );

  QStringList activePages;
  while ( it.current() ) {
    if ( it.current()->parent() == 0 ) {
      PageItem *item = static_cast<PageItem*>( it.current() );
      activePages.append( item->name() );
    }

    ++it;
  }

  return activePages;
}

void KCMDesignerFields::save()
{
  writeActivePages( saveActivePages() );
}

void KCMDesignerFields::defaults()
{
}

void KCMDesignerFields::initGUI()
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setSpacing( KDialog::spacingHint() );
  layout->setMargin( KDialog::marginHint() );

  bool noDesigner = KStandardDirs::findExe("designer").isEmpty();

  if ( noDesigner )
  {
    QString txt =
      i18n("<qt><b>Warning:</b> Qt Designer could not be found. It is probably not "
         "installed. You will only be able to import existing designer files.</qt>");
    QLabel *lbl = new QLabel( txt, this );
    layout->addWidget( lbl );
  }

  QHBoxLayout *hbox = new QHBoxLayout();
  layout->addItem( hbox );
  hbox->setSpacing( KDialog::spacingHint() );

  mPageView = new K3ListView( this );
  mPageView->addColumn( i18n( "Available Pages" ) );
  mPageView->setRootIsDecorated( true );
  mPageView->setAllColumnsShowFocus( true );
  mPageView->setFullWidth( true );
  hbox->addWidget( mPageView );

  Q3GroupBox *box = new Q3GroupBox(1, Qt::Horizontal, i18n("Preview of Selected Page"), this );

  mPagePreview = new QLabel( box );
  mPagePreview->setMinimumWidth( 300 );

  mPageDetails = new QLabel( box );

  hbox->addWidget( box );

  loadUiFiles();

  hbox = new QHBoxLayout();
  layout->addItem( hbox );
  hbox->setSpacing( KDialog::spacingHint() );

  QString cwHowto = i18n("<qt><p>This section allows you to add your own GUI"
                         "  Elements ('<i>Widgets</i>') to store your own values"
                         " into %1. Proceed as described below:</p>"
                         "<ol>"
                         "<li>Click on '<i>Edit with Qt Designer</i>'</li>"
                         "<li>In the dialog, select '<i>Widget</i>', then click <i>OK</i></li>"
                         "<li>Add your widgets to the form</li>"
                         "<li>Save the file in the directory proposed by Qt Designer</li>"
                         "<li>Close Qt Designer</li>"
                         "</ol>"
                         "<p>In case you already have a designer file (*.ui) located"
                         " somewhere on your hard disk, simply choose '<i>Import Page</i>'</p>"
                         "<p><b>Important:</b> The name of each input widget you place within"
                         " the form must start with '<i>X_</i>'; so if you want the widget to"
                         " correspond to your custom entry '<i>X-Foo</i>', set the widget's"
                         " <i>name</i> property to '<i>X_Foo</i>'.</p>"
                         "<p><b>Important:</b> The widget will edit custom fields with an"
                         " application name of %2.  To change the application name"
                         " to be edited, set the widget name in Qt Designer.</p></qt>",
                         applicationName(), applicationName() );

  QLabel *activeLabel = new QLabel(
      i18n( "<a href=\"whatsthis:%1\">How does this work?</a>" , cwHowto), this );
  activeLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);
  connect( activeLabel, SIGNAL(linkActivated(QString)),
           this, SLOT(showWhatsThis(QString)) );
  hbox->addWidget( activeLabel );


  // ### why is this needed? Looks like a KActiveLabel bug...
  activeLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );

  hbox->addStretch( 1 );

  mDeleteButton = new QPushButton( i18n( "Delete Page" ), this);
  mDeleteButton->setEnabled( false );
  hbox->addWidget( mDeleteButton );
  mImportButton = new QPushButton( i18n( "Import Page..." ), this);
  hbox->addWidget( mImportButton );
  mDesignerButton = new QPushButton( i18n( "Edit with Qt Designer..." ), this );
  hbox->addWidget( mDesignerButton );

  if ( noDesigner )
    mDesignerButton->setEnabled( false );

  // FIXME: Why do I have to call show() for all widgets? A this->show() doesn't
  // seem to work.
  mPageView->show();
  box->show();
  activeLabel->show();
  mDeleteButton->show();
  mImportButton->show();
  mDesignerButton->show();
}

void KCMDesignerFields::updatePreview( Q3ListViewItem *item )
{
  bool widgetItemSelected = false;

  if ( item ) {
    if ( item->parent() ) {
      QString details = QString( "<qt><table>"
                                 "<tr><td align=\"right\"><b>%1</b></td><td>%2</td></tr>"
                                 "<tr><td align=\"right\"><b>%3</b></td><td>%4</td></tr>"
                                 "<tr><td align=\"right\"><b>%5</b></td><td>%6</td></tr>"
                                 "<tr><td align=\"right\"><b>%7</b></td><td>%8</td></tr>"
                                 "</table></qt>" )
                                .arg( i18n( "Key:" ) )
                                .arg( item->text( 0 ).replace("X_","X-") )
                                .arg( i18n( "Type:" ) )
                                .arg( item->text( 1 ) )
                                .arg( i18n( "Classname:" ) )
                                .arg( item->text( 2 ) )
                                .arg( i18n( "Description:" ) )
                                .arg( item->text( 3 ) );

      mPageDetails->setText( details );

      PageItem *pageItem = static_cast<PageItem*>( item->parent() );
      mPagePreview->setWindowIcon( pageItem->preview() );
    } else {
      mPageDetails->setText( QString() );

      PageItem *pageItem = static_cast<PageItem*>( item );
      mPagePreview->setWindowIcon( pageItem->preview() );

      widgetItemSelected = true;
    }

    mPagePreview->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  } else {
    mPagePreview->setWindowIcon( QPixmap() );
    mPagePreview->setFrameStyle( 0 );
    mPageDetails->setText( QString() );
  }

  mDeleteButton->setEnabled( widgetItemSelected );
}

void KCMDesignerFields::itemClicked( Q3ListViewItem *item )
{
  if ( !item || item->parent() != 0 )
    return;

  PageItem *pageItem = static_cast<PageItem*>( item );

  if ( pageItem->isOn() != pageItem->isActive() ) {
    emit changed( true );
    pageItem->setIsActive( pageItem->isOn() );
  }
}

void KCMDesignerFields::startDesigner()
{
  QString cmdLine = "designer";

  // check if path exists and create one if not.
  QString cepPath = localUiDir();
  if( !KGlobal::dirs()->exists(cepPath) ) {
    KIO::NetAccess::mkdir( cepPath, this );
  }

  // finally jump there
  chdir(cepPath.toLocal8Bit());

  Q3ListViewItem *item = mPageView->selectedItem();
  if ( item ) {
    PageItem *pageItem = static_cast<PageItem*>( item->parent() ? item->parent() : item );
    cmdLine += ' ' + KShell::quoteArg( pageItem->path() );
  }

  KRun::runCommand( cmdLine, topLevelWidget());
}

void KCMDesignerFields::showWhatsThis(const QString &href)
{
  if (href.startsWith("whatsthis:")) {
    QPoint pos = QCursor::pos();
    QWhatsThis::showText(pos, href.mid(10), this);
  }
}

#include "kcmdesignerfields.moc"
