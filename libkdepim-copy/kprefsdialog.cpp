/*
  This file is part of libkdepim.

  Copyright (c) 2001,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (C) 2005,2008 Allen Winter <winter@kde.org>

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
//krazy:excludeall=tipsandthis

#include "kprefsdialog.h"
#include "ktimeedit.h"
#include "kdateedit.h"

#include <kdeversion.h>
#include <KColorButton>
#include <KComboBox>
#include <KConfigSkeleton>
#include <KDebug>
#include <KFontDialog>
#include <KLineEdit>
#include <KLocale>
#include <KMessageBox>
#include <KPageWidget>
#include <KUrlRequester>

#include <QFont>
#include <QFrame>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <Q3ButtonGroup>
#include <Q3HBox>

#include "kprefsdialog.moc"

using namespace KPIM;

namespace KPrefsWidFactory {
KPrefsWid *create( KConfigSkeletonItem *item, QWidget *parent )
{
  KConfigSkeleton::ItemBool *boolItem =
    dynamic_cast<KConfigSkeleton::ItemBool *>( item );
  if ( boolItem ) {
    return new KPrefsWidBool( boolItem, parent );
  }

  KConfigSkeleton::ItemString *stringItem =
    dynamic_cast<KConfigSkeleton::ItemString *>( item );
  if ( stringItem ) {
    return new KPrefsWidString( stringItem, parent );
  }

  KConfigSkeleton::ItemEnum *enumItem =
    dynamic_cast<KConfigSkeleton::ItemEnum *>( item );
  if ( enumItem ) {
    QList<KConfigSkeleton::ItemEnum::Choice> choices = enumItem->choices();
    if ( choices.isEmpty() ) {
      kError() << "Enum has no choices.";
      return 0;
    } else {
      KPrefsWidRadios *radios = new KPrefsWidRadios( enumItem, parent );
      QList<KConfigSkeleton::ItemEnum::Choice>::ConstIterator it;
      for ( it = choices.begin(); it != choices.end(); ++it ) {
        radios->addRadio( (*it).label );
      }
      return radios;
    }
  }

  KConfigSkeleton::ItemInt *intItem = dynamic_cast<KConfigSkeleton::ItemInt *>( item );
  if ( intItem ) {
    return new KPrefsWidInt( intItem, parent );
  }

  return 0;
}

} // namespace KPrefsWidFactory

QList<QWidget *> KPrefsWid::widgets() const
{
  return QList<QWidget *>();
}

KPrefsWidBool::KPrefsWidBool( KConfigSkeleton::ItemBool *item, QWidget *parent )
  : mItem( item )
{
  mCheck = new QCheckBox( mItem->label(), parent );
  connect( mCheck, SIGNAL(clicked()), SIGNAL(changed()) );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mCheck->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mCheck->setWhatsThis( whatsThis );
  }
}

void KPrefsWidBool::readConfig()
{
  mCheck->setChecked( mItem->value() );
}

void KPrefsWidBool::writeConfig()
{
  mItem->setValue( mCheck->isChecked() );
}

QCheckBox *KPrefsWidBool::checkBox()
{
  return mCheck;
}

QList<QWidget *> KPrefsWidBool::widgets() const
{
  QList<QWidget *> widgets;
  widgets.append( mCheck );
  return widgets;
}

KPrefsWidInt::KPrefsWidInt( KConfigSkeleton::ItemInt *item, QWidget *parent )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label() + ':', parent );
  mSpin = new QSpinBox( parent );
  if ( !mItem->minValue().isNull() ) {
    mSpin->setMinimum( mItem->minValue().toInt() );
  }
  if ( !mItem->maxValue().isNull() ) {
    mSpin->setMaximum( mItem->maxValue().toInt() );
  }
  connect( mSpin, SIGNAL(valueChanged(int)), SIGNAL(changed()) );
  mLabel->setBuddy( mSpin );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mLabel->setToolTip( toolTip );
    mSpin->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mLabel->setWhatsThis( whatsThis );
    mSpin->setWhatsThis( whatsThis );
  }
}

void KPrefsWidInt::readConfig()
{
  mSpin->setValue( mItem->value() );
}

void KPrefsWidInt::writeConfig()
{
  mItem->setValue( mSpin->value() );
}

QLabel *KPrefsWidInt::label()
{
  return mLabel;
}

QSpinBox *KPrefsWidInt::spinBox()
{
  return mSpin;
}

QList<QWidget *> KPrefsWidInt::widgets() const
{
  QList<QWidget *> widgets;
  widgets.append( mLabel );
  widgets.append( mSpin );
  return widgets;
}

KPrefsWidColor::KPrefsWidColor( KConfigSkeleton::ItemColor *item, QWidget *parent )
  : mItem( item )
{
  mButton = new KColorButton( parent );
  connect( mButton, SIGNAL(changed(const QColor&)), SIGNAL(changed()) );
  mLabel = new QLabel( mItem->label() + ':', parent );
  mLabel->setBuddy( mButton );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mButton->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mButton->setWhatsThis( whatsThis );
  }
}

KPrefsWidColor::~KPrefsWidColor()
{
}

void KPrefsWidColor::readConfig()
{
  mButton->setColor( mItem->value() );
}

void KPrefsWidColor::writeConfig()
{
  mItem->setValue( mButton->color() );
}

QLabel *KPrefsWidColor::label()
{
  return mLabel;
}

KColorButton *KPrefsWidColor::button()
{
  return mButton;
}

KPrefsWidFont::KPrefsWidFont( KConfigSkeleton::ItemFont *item,
                              QWidget *parent, const QString &sampleText )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label() + ':', parent );

  mPreview = new QLabel( sampleText, parent );
  mPreview->setFrameStyle( QFrame::Panel | QFrame::Sunken );

  mButton = new QPushButton( i18n( "Choose..." ), parent );
  connect( mButton, SIGNAL(clicked()), SLOT(selectFont()) );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mPreview->setToolTip( toolTip );
    mButton->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mPreview->setWhatsThis( whatsThis );
    mButton->setWhatsThis( whatsThis );
  }
}

KPrefsWidFont::~KPrefsWidFont()
{
}

void KPrefsWidFont::readConfig()
{
  mPreview->setFont( mItem->value() );
}

void KPrefsWidFont::writeConfig()
{
  mItem->setValue( mPreview->font() );
}

QLabel *KPrefsWidFont::label()
{
  return mLabel;
}

QFrame *KPrefsWidFont::preview()
{
  return mPreview;
}

QPushButton *KPrefsWidFont::button()
{
  return mButton;
}

void KPrefsWidFont::selectFont()
{
  QFont myFont( mPreview->font() );
  int result = KFontDialog::getFont( myFont );
  if ( result == KFontDialog::Accepted ) {
    mPreview->setFont( myFont );
    emit changed();
  }
}

KPrefsWidTime::KPrefsWidTime( KConfigSkeleton::ItemDateTime *item, QWidget *parent )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label() + ':', parent );
  mTimeEdit = new KTimeEdit( parent );
  mLabel->setBuddy( mTimeEdit );
  connect( mTimeEdit, SIGNAL(timeChanged(QTime)), SIGNAL(changed()) );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mTimeEdit->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mTimeEdit->setWhatsThis( whatsThis );
  }
}

void KPrefsWidTime::readConfig()
{
  mTimeEdit->setTime( mItem->value().time() );
}

void KPrefsWidTime::writeConfig()
{
  // Don't overwrite the date value of the QDateTime, so we can use a
  // KPrefsWidTime and a KPrefsWidDate on the same config entry!
  QDateTime dt( mItem->value() );
  dt.setTime( mTimeEdit->getTime() );
  mItem->setValue( dt );
}

QLabel *KPrefsWidTime::label()
{
  return mLabel;
}

KTimeEdit *KPrefsWidTime::timeEdit()
{
  return mTimeEdit;
}

KPrefsWidDuration::KPrefsWidDuration( KConfigSkeleton::ItemDateTime *item, QWidget *parent )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label() + ':', parent );
  mTimeEdit = new QTimeEdit( parent );
  mLabel->setBuddy( mTimeEdit );
  mTimeEdit->setDisplayFormat( "hh:mm:ss" );
  mTimeEdit->setMinimumTime( QTime( 0, 1 ) ); // [1 min]
  mTimeEdit->setMaximumTime( QTime( 24, 0 ) ); // [24 hr]
  connect( mTimeEdit, SIGNAL(timeChanged(const QTime&)), SIGNAL(changed()) );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mTimeEdit->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mTimeEdit->setWhatsThis( whatsThis );
  }
}

void KPrefsWidDuration::readConfig()
{
  mTimeEdit->setTime( mItem->value().time() );
}

void KPrefsWidDuration::writeConfig()
{
  QDateTime dt( mItem->value() );
  dt.setTime( mTimeEdit->time() );
  mItem->setValue( dt );
}

QLabel *KPrefsWidDuration::label()
{
  return mLabel;
}

QTimeEdit *KPrefsWidDuration::timeEdit()
{
  return mTimeEdit;
}

KPrefsWidDate::KPrefsWidDate( KConfigSkeleton::ItemDateTime *item, QWidget *parent )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label() + ':', parent );
  mDateEdit = new KDateEdit( parent );
  mLabel->setBuddy( mDateEdit );
  connect( mDateEdit, SIGNAL(dateChanged(const QDate&)), SIGNAL(changed()) );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mDateEdit->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mDateEdit->setWhatsThis( whatsThis );
  }
}

void KPrefsWidDate::readConfig()
{
  if ( !mItem->value().date().isValid() ) {
    mItem->setValue( QDateTime::currentDateTime() );
  }
  mDateEdit->setDate( mItem->value().date() );
}

void KPrefsWidDate::writeConfig()
{
  QDateTime dt( mItem->value() );
  dt.setDate( mDateEdit->date() );
  mItem->setValue( dt );
  if ( !mItem->value().date().isValid() ) {
    mItem->setValue( QDateTime::currentDateTime() );
  }
}

QLabel *KPrefsWidDate::label()
{
  return mLabel;
}

KDateEdit *KPrefsWidDate::dateEdit()
{
  return mDateEdit;
}

KPrefsWidRadios::KPrefsWidRadios( KConfigSkeleton::ItemEnum *item, QWidget *parent )
  : mItem( item )
{
  mBox = new Q3ButtonGroup( 1, Qt::Horizontal, mItem->label(), parent );
  connect( mBox, SIGNAL(clicked(int)), SIGNAL(changed()) );
}

KPrefsWidRadios::~KPrefsWidRadios()
{
}

void KPrefsWidRadios::addRadio( const QString &text, const QString &toolTip,
                                const QString &whatsThis )
{
  QRadioButton *r = new QRadioButton( text, mBox );
  if ( !toolTip.isEmpty() ) {
    r->setToolTip( toolTip );
  }
  if ( !whatsThis.isEmpty() ) {
    r->setWhatsThis( whatsThis );
  }
}

Q3ButtonGroup *KPrefsWidRadios::groupBox()
{
  return mBox;
}

void KPrefsWidRadios::readConfig()
{
  mBox->setButton( mItem->value() );
}

void KPrefsWidRadios::writeConfig()
{
  mItem->setValue( mBox->id( mBox->selected() ) );
}

QList<QWidget *> KPrefsWidRadios::widgets() const
{
  QList<QWidget *> w;
  w.append( mBox );
  return w;
}

KPrefsWidCombo::KPrefsWidCombo( KConfigSkeleton::ItemEnum *item, QWidget *parent )
  : mItem( item )
{
  Q3HBox *hbox = new Q3HBox( parent );
  new QLabel( mItem->label(), hbox );
  mCombo = new KComboBox( hbox );
  connect( mCombo, SIGNAL(activated(int)), SIGNAL(changed()) );
}

KPrefsWidCombo::~KPrefsWidCombo()
{
}

void KPrefsWidCombo::readConfig()
{
  mCombo->setCurrentIndex( mItem->value() );
}

void KPrefsWidCombo::writeConfig()
{
  mItem->setValue( mCombo->currentIndex() );
}

QList<QWidget *> KPrefsWidCombo::widgets() const
{
  QList<QWidget *> w;
  w.append( mCombo );
  return w;
}

KComboBox *KPrefsWidCombo::comboBox()
{
  return mCombo;
}

KPrefsWidString::KPrefsWidString( KConfigSkeleton::ItemString *item, QWidget *parent,
                                  KLineEdit::EchoMode echomode )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label() + ':', parent );
  mEdit = new KLineEdit( parent );
  mLabel->setBuddy( mEdit );
  connect( mEdit, SIGNAL(textChanged(const QString&)), SIGNAL(changed()) );
  mEdit->setEchoMode( echomode );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mEdit->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mEdit->setWhatsThis( whatsThis );
  }
}

KPrefsWidString::~KPrefsWidString()
{
}

void KPrefsWidString::readConfig()
{
  mEdit->setText( mItem->value() );
}

void KPrefsWidString::writeConfig()
{
  mItem->setValue( mEdit->text() );
}

QLabel *KPrefsWidString::label()
{
  return mLabel;
}

KLineEdit *KPrefsWidString::lineEdit()
{
  return mEdit;
}

QList<QWidget *> KPrefsWidString::widgets() const
{
  QList<QWidget *> widgets;
  widgets.append( mLabel );
  widgets.append( mEdit );
  return widgets;
}

KPrefsWidPath::KPrefsWidPath( KConfigSkeleton::ItemPath *item, QWidget *parent,
                              const QString &filter, KFile::Modes mode )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label() + ':', parent );
  mURLRequester = new KUrlRequester( parent );
  mLabel->setBuddy( mURLRequester );
  mURLRequester->setMode( mode );
  mURLRequester->setFilter( filter );
  connect( mURLRequester, SIGNAL(textChanged(const QString&)), SIGNAL(changed()) );
#if KDE_IS_VERSION( 4, 1, 63 )
  QString toolTip = mItem->toolTip();
  if ( !toolTip.isEmpty() ) {
    mURLRequester->setToolTip( toolTip );
  }
#endif
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    mURLRequester->setWhatsThis( whatsThis );
  }
}

KPrefsWidPath::~KPrefsWidPath()
{
}

void KPrefsWidPath::readConfig()
{
  mURLRequester->setPath( mItem->value() );
}

void KPrefsWidPath::writeConfig()
{
  mItem->setValue( mURLRequester->url().path() );
}

QLabel *KPrefsWidPath::label()
{
  return mLabel;
}

KUrlRequester *KPrefsWidPath::urlRequester()
{
  return mURLRequester;
}

QList<QWidget *> KPrefsWidPath::widgets() const
{
  QList<QWidget *> widgets;
  widgets.append( mLabel );
  widgets.append( mURLRequester );
  return widgets;
}

KPrefsWidManager::KPrefsWidManager( KConfigSkeleton *prefs )
  : mPrefs( prefs )
{
}

KPrefsWidManager::~KPrefsWidManager()
{
  qDeleteAll( mPrefsWids );
  mPrefsWids.clear();
}

void KPrefsWidManager::addWid( KPrefsWid *wid )
{
  mPrefsWids.append( wid );
}

KPrefsWidBool *KPrefsWidManager::addWidBool( KConfigSkeleton::ItemBool *item,
                                             QWidget *parent )
{
  KPrefsWidBool *w = new KPrefsWidBool( item, parent );
  addWid( w );
  return w;
}

KPrefsWidTime *KPrefsWidManager::addWidTime( KConfigSkeleton::ItemDateTime *item,
                                             QWidget *parent )
{
  KPrefsWidTime *w = new KPrefsWidTime( item, parent );
  addWid( w );
  return w;
}

KPrefsWidDuration *KPrefsWidManager::addWidDuration( KConfigSkeleton::ItemDateTime *item,
                                                     QWidget *parent )
{
  KPrefsWidDuration *w = new KPrefsWidDuration( item, parent );
  addWid( w );
  return w;
}

KPrefsWidDate *KPrefsWidManager::addWidDate( KConfigSkeleton::ItemDateTime *item,
                                             QWidget *parent )
{
  KPrefsWidDate *w = new KPrefsWidDate( item, parent );
  addWid( w );
  return w;
}

KPrefsWidColor *KPrefsWidManager::addWidColor( KConfigSkeleton::ItemColor *item,
                                               QWidget *parent )
{
  KPrefsWidColor *w = new KPrefsWidColor( item, parent );
  addWid( w );
  return w;
}

KPrefsWidRadios *KPrefsWidManager::addWidRadios( KConfigSkeleton::ItemEnum *item,
                                                 QWidget *parent )
{
  KPrefsWidRadios *w = new KPrefsWidRadios( item, parent );
#if KDE_IS_VERSION( 4, 1, 63 )
  QList<KConfigSkeleton::ItemEnum::Choice2> choices;
  choices = item->choices2();
  QList<KConfigSkeleton::ItemEnum::Choice2>::ConstIterator it;
  for ( it = choices.begin(); it != choices.end(); ++it ) {
    w->addRadio( (*it).label, (*it).toolTip, (*it).whatsThis );
  }
#else
  QList<KConfigSkeleton::ItemEnum::Choice> choices;
  choices = item->choices();
  QList<KConfigSkeleton::ItemEnum::Choice>::ConstIterator it;
  for ( it = choices.begin(); it != choices.end(); ++it ) {
    w->addRadio( (*it).label, QString(), (*it).whatsThis );
  }
#endif
  addWid( w );
  return w;
}

KPrefsWidCombo *KPrefsWidManager::addWidCombo( KConfigSkeleton::ItemEnum *item,
                                               QWidget *parent )
{
  KPrefsWidCombo *w = new KPrefsWidCombo( item, parent );
  QList<KConfigSkeleton::ItemEnum::Choice> choices;
  choices = item->choices();
  QList<KConfigSkeleton::ItemEnum::Choice>::ConstIterator it;
  for ( it = choices.begin(); it != choices.end(); ++it ) {
    w->comboBox()->addItem( (*it).label );
  }
  addWid( w );
  return w;
}

KPrefsWidString *KPrefsWidManager::addWidString( KConfigSkeleton::ItemString *item,
                                                 QWidget *parent )
{
  KPrefsWidString *w = new KPrefsWidString( item, parent, KLineEdit::Normal );
  addWid( w );
  return w;
}

KPrefsWidPath *KPrefsWidManager::addWidPath( KConfigSkeleton::ItemPath *item,
                                             QWidget *parent,
                                             const QString &filter,
                                             KFile::Modes mode )
{
  KPrefsWidPath *w = new KPrefsWidPath( item, parent, filter, mode );
  addWid( w );
  return w;
}

KPrefsWidString *KPrefsWidManager::addWidPassword( KConfigSkeleton::ItemString *item,
                                                   QWidget *parent )
{
  KPrefsWidString *w = new KPrefsWidString( item, parent, KLineEdit::Password );
  addWid( w );
  return w;
}

KPrefsWidFont *KPrefsWidManager::addWidFont( KConfigSkeleton::ItemFont *item,
                                             QWidget *parent,
                                             const QString &sampleText )
{
  KPrefsWidFont *w = new KPrefsWidFont( item, parent, sampleText );
  addWid( w );
  return w;
}

KPrefsWidInt *KPrefsWidManager::addWidInt( KConfigSkeleton::ItemInt *item,
                                           QWidget *parent )
{
  KPrefsWidInt *w = new KPrefsWidInt( item, parent );
  addWid( w );
  return w;
}

void KPrefsWidManager::setWidDefaults()
{
  bool tmp = mPrefs->useDefaults( true );
  readWidConfig();
  mPrefs->useDefaults( tmp );
}

void KPrefsWidManager::readWidConfig()
{
  QList<KPrefsWid*>::Iterator it;
  for ( it = mPrefsWids.begin(); it != mPrefsWids.end(); ++it ) {
    (*it)->readConfig();
  }
}

void KPrefsWidManager::writeWidConfig()
{
  QList<KPrefsWid*>::Iterator it;
  for ( it = mPrefsWids.begin(); it != mPrefsWids.end(); ++it ) {
    (*it)->writeConfig();
  }

  mPrefs->writeConfig();
}

KPrefsDialog::KPrefsDialog( KConfigSkeleton *prefs, QWidget *parent, bool modal )
  : KPageDialog( parent ),
    KPrefsWidManager( prefs )
{
  setFaceType( List );
  setCaption( i18n( "Preferences" ) );
  setButtons( Ok|Apply|Cancel|Default );
  setDefaultButton( Ok );
  setModal( modal );
  showButtonSeparator( true );
  connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
  connect( this, SIGNAL(applyClicked()), SLOT(slotApply()) );
  connect( this, SIGNAL(defaultClicked()), SLOT(slotDefault()) );
  connect( this, SIGNAL(cancelClicked()), SLOT(reject()) );
}

KPrefsDialog::~KPrefsDialog()
{

}

void KPrefsDialog::autoCreate()
{
  KConfigSkeletonItem::List items = prefs()->items();

  QMap<QString,QWidget *> mGroupPages;
  QMap<QString,QGridLayout *> mGroupLayouts;
  QMap<QString,int> mCurrentRows;

  KConfigSkeletonItem::List::ConstIterator it;
  for ( it = items.begin(); it != items.end(); ++it ) {
    QString group = (*it)->group();
    QString name = (*it)->name();

    QWidget *page;
    QGridLayout *layout;
    int currentRow;
    if ( !mGroupPages.contains( group ) ) {
      page = new QWidget( this );
      addPage( page, group );
      layout = new QGridLayout( page );
      mGroupPages.insert( group, page );
      mGroupLayouts.insert( group, layout );
      currentRow = 0;
      mCurrentRows.insert( group, currentRow );
    } else {
      page = mGroupPages[ group ];
      layout = mGroupLayouts[ group ];
      currentRow = mCurrentRows[ group ];
    }

    KPrefsWid *wid = KPrefsWidFactory::create( *it, page );

    if ( wid ) {
      QList<QWidget *> widgets = wid->widgets();
      if ( widgets.count() == 1 ) {
        layout->addWidget( widgets[ 0 ], currentRow, currentRow, 0, 1 );
      } else if ( widgets.count() == 2 ) {
        layout->addWidget( widgets[ 0 ], currentRow, 0 );
        layout->addWidget( widgets[ 1 ], currentRow, 1 );
      } else {
        kError() <<"More widgets than expected:" << widgets.count();
      }

      if ( (*it)->isImmutable() ) {
        QList<QWidget *>::Iterator it2;
        for ( it2 = widgets.begin(); it2 != widgets.end(); ++it2 ) {
          (*it2)->setEnabled( false );
        }
      }
      addWid( wid );
      mCurrentRows.insert( group, ++currentRow );
    }
  }

  readConfig();
}

void KPrefsDialog::setDefaults()
{
  setWidDefaults();
}

void KPrefsDialog::readConfig()
{
  readWidConfig();
  usrReadConfig();
}

void KPrefsDialog::writeConfig()
{
  writeWidConfig();
  usrWriteConfig();
  readConfig();
}

void KPrefsDialog::slotApply()
{
  writeConfig();

  emit configChanged();
}

void KPrefsDialog::slotOk()
{
  slotApply();
  accept();
}

void KPrefsDialog::slotDefault()
{
  if ( KMessageBox::warningContinueCancel(
         this,
         i18n( "You are about to set all preferences to default values. "
               "All custom modifications will be lost." ),
         i18n( "Setting Default Preferences" ),
         KGuiItem( i18n( "Reset to Defaults" ) ) ) == KMessageBox::Continue ) {
    setDefaults();
  }
}

KPrefsModule::KPrefsModule( KConfigSkeleton *prefs, const KComponentData &instance,
                            QWidget *parent, const QVariantList &args )
  : KCModule( instance, parent, args ),
    KPrefsWidManager( prefs )
{
  emit changed( false );
}

void KPrefsModule::addWid( KPrefsWid *wid )
{
  KPrefsWidManager::addWid( wid );
  connect( wid, SIGNAL(changed()), SLOT(slotWidChanged()) );
}

void KPrefsModule::slotWidChanged()
{
  emit changed( true );
}

void KPrefsModule::load()
{
  readWidConfig();
  usrReadConfig();

  emit changed( false );
}

void KPrefsModule::save()
{
  writeWidConfig();
  usrWriteConfig();
}

void KPrefsModule::defaults()
{
  setWidDefaults();

  emit changed( true );
}
