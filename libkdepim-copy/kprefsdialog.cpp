/*
    This file is part of libkdepim.

    Copyright (c) 2001,2003 Cornelius Schumacher <schumacher@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <qlayout.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qlineedit.h>
#include <qfont.h>
#include <qspinbox.h>
#include <qframe.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>

#include <kcolorbutton.h>
#include <kdebug.h>
#include <klocale.h>
#include <kfontdialog.h>
#include <kmessagebox.h>
#include <kconfigskeleton.h>
#include <kurlrequester.h> 
#include "ktimeedit.h"
#include "kdateedit.h"

#include "kprefsdialog.h"
#include "kprefsdialog.moc"

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
    QValueList<KConfigSkeleton::ItemEnum::Choice> choices = enumItem->choices();
    if ( choices.isEmpty() ) {
      kdError() << "KPrefsWidFactory::create(): Enum has no choices." << endl;
      return 0;
    } else {
      KPrefsWidRadios *radios = new KPrefsWidRadios( enumItem, parent );
      QValueList<KConfigSkeleton::ItemEnum::Choice>::ConstIterator it;
      for( it = choices.begin(); it != choices.end(); ++it ) {
        radios->addRadio( (*it).label );
      }
      return radios;
    }
  }

  KConfigSkeleton::ItemInt *intItem =
      dynamic_cast<KConfigSkeleton::ItemInt *>( item );
  if ( intItem ) {
    return new KPrefsWidInt( intItem, parent );
  }

  return 0;
}

}


QValueList<QWidget *> KPrefsWid::widgets() const
{
  return QValueList<QWidget *>();
}


KPrefsWidBool::KPrefsWidBool( KConfigSkeleton::ItemBool *item, QWidget *parent )
  : mItem( item )
{
  mCheck = new QCheckBox( item->label(), parent);
  connect( mCheck, SIGNAL( clicked() ), SIGNAL( changed() ) );
  if ( !item->whatsThis().isNull() ) {
    QWhatsThis::add( mCheck, item->whatsThis() );
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

QValueList<QWidget *> KPrefsWidBool::widgets() const
{
  QValueList<QWidget *> widgets;
  widgets.append( mCheck );
  return widgets;
}


KPrefsWidInt::KPrefsWidInt( KConfigSkeleton::ItemInt *item,
                            QWidget *parent )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label()+':', parent );
  mSpin = new QSpinBox( parent );
  if ( !item->minValue().isNull() ) {
    mSpin->setMinValue( item->minValue().toInt() );
  }
  if ( !item->maxValue().isNull() ) {
    mSpin->setMaxValue( item->maxValue().toInt() );
  }
  connect( mSpin, SIGNAL( valueChanged( int ) ), SIGNAL( changed() ) );
  mLabel->setBuddy( mSpin );
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isEmpty() ) {
    QWhatsThis::add( mLabel, whatsThis );
    QWhatsThis::add( mSpin, whatsThis );
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

QValueList<QWidget *> KPrefsWidInt::widgets() const
{
  QValueList<QWidget *> widgets;
  widgets.append( mLabel );
  widgets.append( mSpin );
  return widgets;
}


KPrefsWidColor::KPrefsWidColor( KConfigSkeleton::ItemColor *item,
                                QWidget *parent )
  : mItem( item )
{
  mButton = new KColorButton( parent );
  connect( mButton, SIGNAL( changed( const QColor & ) ), SIGNAL( changed() ) );
  mLabel = new QLabel( mButton, mItem->label()+':', parent );
  QString whatsThis = mItem->whatsThis();
  if (!whatsThis.isNull()) {
    QWhatsThis::add(mButton, whatsThis);
  }
}

KPrefsWidColor::~KPrefsWidColor()
{
//  kdDebug(5300) << "KPrefsWidColor::~KPrefsWidColor()" << endl;
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
  mLabel = new QLabel( mItem->label()+':', parent );

  mPreview = new QLabel( sampleText, parent );
  mPreview->setFrameStyle( QFrame::Panel | QFrame::Sunken );

  mButton = new QPushButton( i18n("Choose..."), parent );
  connect( mButton, SIGNAL( clicked() ), SLOT( selectFont() ) );
  QString whatsThis = mItem->whatsThis();
  if (!whatsThis.isNull()) {
    QWhatsThis::add(mPreview, whatsThis);
    QWhatsThis::add(mButton, whatsThis);
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
  QFont myFont(mPreview->font());
  int result = KFontDialog::getFont(myFont);
  if (result == KFontDialog::Accepted) {
    mPreview->setFont(myFont);
    emit changed();
  }
}


KPrefsWidTime::KPrefsWidTime( KConfigSkeleton::ItemDateTime *item,
                              QWidget *parent )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label()+':', parent );
  mTimeEdit = new KTimeEdit( parent );
  connect( mTimeEdit, SIGNAL( timeChanged( QTime ) ), SIGNAL( changed() ) );
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isNull() ) {
    QWhatsThis::add( mTimeEdit, whatsThis );
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


KPrefsWidDate::KPrefsWidDate( KConfigSkeleton::ItemDateTime *item,
                              QWidget *parent )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label()+':', parent );
  mDateEdit = new KDateEdit( parent );
  connect( mDateEdit, SIGNAL( dateChanged( QDate ) ), SIGNAL( changed() ) );
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isNull() ) {
    QWhatsThis::add( mDateEdit, whatsThis );
  }
}

void KPrefsWidDate::readConfig()
{
  mDateEdit->setDate( mItem->value().date() );
}

void KPrefsWidDate::writeConfig()
{
  QDateTime dt( mItem->value() );
  dt.setDate( mDateEdit->date() );
  mItem->setValue( dt );
}

QLabel *KPrefsWidDate::label()
{
  return mLabel;
}

KDateEdit *KPrefsWidDate::dateEdit()
{
  return mDateEdit;
}


KPrefsWidRadios::KPrefsWidRadios( KConfigSkeleton::ItemEnum *item,
                                  QWidget *parent )
  : mItem( item )
{
  mBox = new QButtonGroup( 1, Qt::Horizontal, mItem->label(), parent );
  connect( mBox, SIGNAL( clicked( int ) ), SIGNAL( changed() ) );
}

KPrefsWidRadios::~KPrefsWidRadios()
{
}

void KPrefsWidRadios::addRadio(const QString &text, const QString &whatsThis)
{
  QRadioButton *r = new QRadioButton(text,mBox);
  if (!whatsThis.isNull()) {
    QWhatsThis::add(r, whatsThis);
  }
}

QButtonGroup *KPrefsWidRadios::groupBox()
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

QValueList<QWidget *> KPrefsWidRadios::widgets() const
{
  QValueList<QWidget *> w;
  w.append( mBox );
  return w;
}


KPrefsWidString::KPrefsWidString( KConfigSkeleton::ItemString *item,
                                  QWidget *parent,
                                  QLineEdit::EchoMode echomode )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label()+':', parent );
  mEdit = new QLineEdit( parent );
  connect( mEdit, SIGNAL( textChanged( const QString & ) ),
           SIGNAL( changed() ) );
  mEdit->setEchoMode( echomode );
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isNull() ) {
    QWhatsThis::add( mEdit, whatsThis );
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

QLineEdit *KPrefsWidString::lineEdit()
{
  return mEdit;
}

QValueList<QWidget *> KPrefsWidString::widgets() const
{
  QValueList<QWidget *> widgets;
  widgets.append( mLabel );
  widgets.append( mEdit );
  return widgets;
}


KPrefsWidPath::KPrefsWidPath( KConfigSkeleton::ItemPath *item, QWidget *parent, 
                              const QString &filter, uint mode )
  : mItem( item )
{
  mLabel = new QLabel( mItem->label()+':', parent );
  mURLRequester = new KURLRequester( parent );
  mURLRequester->setMode( mode );
  mURLRequester->setFilter( filter );
  connect( mURLRequester, SIGNAL( textChanged( const QString & ) ),
           SIGNAL( changed() ) );
  QString whatsThis = mItem->whatsThis();
  if ( !whatsThis.isNull() ) {
    QWhatsThis::add( mURLRequester, whatsThis );
  }
}

KPrefsWidPath::~KPrefsWidPath()
{
}

void KPrefsWidPath::readConfig()
{
  mURLRequester->setURL( mItem->value() );
}

void KPrefsWidPath::writeConfig()
{
  mItem->setValue( mURLRequester->url() );
}

QLabel *KPrefsWidPath::label()
{
  return mLabel;
}

KURLRequester *KPrefsWidPath::urlRequester()
{
  return mURLRequester;
}

QValueList<QWidget *> KPrefsWidPath::widgets() const
{
  QValueList<QWidget *> widgets;
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
  QValueList<KConfigSkeleton::ItemEnum::Choice> choices;
  choices = item->choices();
  QValueList<KConfigSkeleton::ItemEnum::Choice>::ConstIterator it;
  for( it = choices.begin(); it != choices.end(); ++it ) {
    w->addRadio( (*it).label, (*it).whatsThis );
  }
  addWid( w );
  return w;
}

KPrefsWidString *KPrefsWidManager::addWidString( KConfigSkeleton::ItemString *item,
                                                 QWidget *parent )
{
  KPrefsWidString *w = new KPrefsWidString( item, parent,
                                            QLineEdit::Normal );
  addWid( w );
  return w;
}

KPrefsWidPath *KPrefsWidManager::addWidPath( KConfigSkeleton::ItemPath *item,
                                             QWidget *parent, const QString &filter, uint mode )
{
  KPrefsWidPath *w = new KPrefsWidPath( item, parent, filter, mode );
  addWid( w );
  return w;
}

KPrefsWidString *KPrefsWidManager::addWidPassword( KConfigSkeleton::ItemString *item,
                                                   QWidget *parent )
{
  KPrefsWidString *w = new KPrefsWidString( item, parent, QLineEdit::Password );
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
  kdDebug() << "KPrefsWidManager::setWidDefaults()" << endl;

  bool tmp = mPrefs->useDefaults( true );

  readWidConfig();

  mPrefs->useDefaults( tmp );
}

void KPrefsWidManager::readWidConfig()
{
  kdDebug(5310) << "KPrefsWidManager::readWidConfig()" << endl;

  KPrefsWid *wid;
  for( wid = mPrefsWids.first(); wid; wid = mPrefsWids.next() ) {
    wid->readConfig();
  }
}

void KPrefsWidManager::writeWidConfig()
{
  kdDebug(5310) << "KPrefsWidManager::writeWidConfig()" << endl;

  KPrefsWid *wid;
  for( wid = mPrefsWids.first(); wid; wid = mPrefsWids.next() ) {
    wid->writeConfig();
  }

  mPrefs->writeConfig();
}


KPrefsDialog::KPrefsDialog( KConfigSkeleton *prefs, QWidget *parent, char *name,
                            bool modal )
  : KDialogBase(IconList,i18n("Preferences"),Ok|Apply|Cancel|Default,Ok,parent,
                name,modal,true),
    KPrefsWidManager( prefs )
{
// TODO: This seems to cause a crash on exit. Investigate later.
//  mPrefsWids.setAutoDelete(true);

//  connect(this,SIGNAL(defaultClicked()),SLOT(setDefaults()));
  connect(this,SIGNAL(cancelClicked()),SLOT(reject()));
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
  for( it = items.begin(); it != items.end(); ++it ) {
    QString group = (*it)->group();
    QString name = (*it)->name();

    kdDebug() << "ITEMS: " << (*it)->name() << endl;

    QWidget *page;
    QGridLayout *layout;
    int currentRow;
    if ( !mGroupPages.contains( group ) ) {
      page = addPage( group );
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
      QValueList<QWidget *> widgets = wid->widgets();
      if ( widgets.count() == 1 ) {
        layout->addMultiCellWidget( widgets[ 0 ],
                                    currentRow, currentRow, 0, 1 );
      } else if ( widgets.count() == 2 ) {
        layout->addWidget( widgets[ 0 ], currentRow, 0 );
        layout->addWidget( widgets[ 1 ], currentRow, 1 );
      } else {
        kdError() << "More widgets than expected: " << widgets.count() << endl;
      }

      if ( (*it)->isImmutable() ) {
        QValueList<QWidget *>::Iterator it2;
        for( it2 = widgets.begin(); it2 != widgets.end(); ++it2 ) {
          (*it2)->setEnabled( false );
        }
      }

      addWid( wid );

      mCurrentRows.replace( group, ++currentRow );
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
  kdDebug() << "KPrefsDialog::slotDefault()" << endl;

  if (KMessageBox::warningContinueCancel(this,
      i18n("You are about to set all preferences to default values. All "
      "custom modifications will be lost."),i18n("Setting Default Preferences"),
      i18n("Reset to Defaults"))
    == KMessageBox::Continue) setDefaults();
}


KPrefsModule::KPrefsModule( KConfigSkeleton *prefs, QWidget *parent,
                            const char *name )
  : KCModule( parent, name ),
    KPrefsWidManager( prefs )
{
  emit changed( false );
}

void KPrefsModule::addWid( KPrefsWid *wid )
{
  KPrefsWidManager::addWid( wid );

  connect( wid, SIGNAL( changed() ), SLOT( slotWidChanged() ) );
}

void KPrefsModule::slotWidChanged()
{
  kdDebug(5310) << "KPrefsModule::slotWidChanged()" << endl;

  emit changed( true );
}

void KPrefsModule::load()
{
  kdDebug(5310) << "KPrefsModule::load()" << endl;

  readWidConfig();

  usrReadConfig();

  emit changed( false );
}

void KPrefsModule::save()
{
  kdDebug(5310) << "KPrefsModule::save()" << endl;

  writeWidConfig();

  usrWriteConfig();
}

void KPrefsModule::defaults()
{
  setWidDefaults();

  emit changed( true );
}
