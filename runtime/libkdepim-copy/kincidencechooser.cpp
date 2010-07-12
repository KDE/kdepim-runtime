/*
  This file is part of libkdepim.

  Copyright (c) 2004 Lutz Rogowski <rogowski@kde.org>
  Copyright (c) 2009 Allen Winter <winter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "kincidencechooser.h"
using namespace KPIM;

#include <kcal/incidence.h>
#include <kcal/incidenceformatter.h>
using namespace KCal;

#include <KHBox>
#include <KLocale>

#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>

int KIncidenceChooser::chooseMode = KIncidenceChooser::ask ;

KIncidenceChooser::KIncidenceChooser( QWidget *parent )
  : KDialog( parent )
{
    setModal( true );
    QWidget *topFrame = mainWidget();
    QGridLayout *topLayout = new QGridLayout( topFrame );
    topLayout->setMargin( 5 );
    topLayout->setSpacing( 3 );

    int iii = 0;
    setWindowTitle( i18nc( "@title:window", "Conflict Detected" ) );
    QLabel *lab;
    lab = new QLabel(
      i18nc( "@info",
             "A conflict was detected. This probably means someone edited "
             "the same incidence on the server while you changed it locally."
             "<nl/><note>You have to check mail again to apply your changes "
             "to the server.</note>" ), topFrame );
    lab->setWordWrap( true );
    topLayout->addWidget( lab, iii, 0, 1, 3 );
    ++iii;
    KHBox *b_box = new KHBox( topFrame );
    topLayout->addWidget( b_box, iii, 0, 1, 3 );
    ++iii;
    QPushButton *locBut = new QPushButton( i18nc( "@action:button", "Take Local" ), b_box );
    connect( locBut, SIGNAL(clicked()), this, SLOT(takeIncidence1()) );
    locBut->setToolTip(
      i18nc( "@info:tooltip", "Take your local copy of the incidence" ) );
    locBut->setWhatsThis(
      i18nc( "@info:whatsthis",
             "A conflict was detected between your local copy of the incidence "
             "and the remote incidence on the server. Press the \"Take Local\" "
             "button to make sure your local copy is used." ) );

    QPushButton *remBut = new QPushButton( i18nc( "@action:button", "Take New" ), b_box );
    connect( remBut, SIGNAL(clicked()), this, SLOT(takeIncidence2()) );
    remBut->setToolTip(
      i18nc( "@info:tooltip", "Take the server copy of the incidence" ) );
    remBut->setWhatsThis(
      i18nc( "@info:whatsthis",
             "A conflict was detected between your local copy of the incidence "
             "and the remote incidence on the server. Press the \"Take New\" "
             "button to use the server copy, thereby overwriting your local copy" ) );

    QPushButton *bothBut = new QPushButton( i18nc( "@action:button", "Take Both" ), b_box );
    bothBut->setFocus(); //kolab/issue4147:  "Take Both" should be default
    connect( bothBut, SIGNAL(clicked()), this, SLOT(takeBoth()) );
    bothBut->setToolTip(
      i18nc( "@info:tooltip", "Take both copies of the incidence" ) );
    bothBut->setWhatsThis(
      i18nc( "@info:whatsthis",
             "A conflict was detected between your local copy of the incidence "
             "and the remote incidence on the server. Press the \"Take Both\" "
             "button to keep both the local and the server copies." ) );

    topLayout->setSpacing( spacingHint() );
    topLayout->setMargin( marginHint() );

    mInc1lab = new QLabel( i18nc( "@label", "Local incidence" ), topFrame );
    topLayout->addWidget( mInc1lab, iii, 0 );

    mInc1Sumlab = new QLabel( i18nc( "@label", "Local incidence summary" ), topFrame );
    topLayout->addWidget( mInc1Sumlab, iii, 1, 1, 2 );
    ++iii;

    topLayout->addWidget( new QLabel( i18nc( "@label", "Last modified:" ), topFrame ), iii, 0 );

    mMod1lab = new QLabel( i18nc( "@label", "Set Last modified" ), topFrame );
    topLayout->addWidget( mMod1lab, iii, 1 );

    mShowDetails1 = new QPushButton( i18nc( "@action:button", "Show Details" ), topFrame );
    mShowDetails1->setToolTip(
      i18nc( "@info:tooltip", "Hide/Show incidence details" ) );
    mShowDetails1->setWhatsThis(
      i18nc( "@info:whatsthis",
             "Press this button to toggle the incidence details display." ) );
    connect( mShowDetails1, SIGNAL(clicked()), this, SLOT(showIncidence1()) );
    topLayout->addWidget( mShowDetails1, iii, 2 );
    ++iii;

    mInc2lab = new QLabel( i18nc( "@label", "Local incidence" ), topFrame );
    topLayout->addWidget( mInc2lab, iii, 0 );

    mInc2Sumlab = new QLabel( i18nc( "@label", "Local incidence summary" ), topFrame );
    topLayout->addWidget( mInc2Sumlab, iii, 1, 1, 2 );
    ++iii;

    topLayout->addWidget( new QLabel( i18nc( "@label", "Last modified:" ), topFrame ), iii, 0 );

    mMod2lab = new QLabel( i18nc( "@label", "Set Last modified" ), topFrame );
    topLayout->addWidget( mMod2lab, iii, 1 );

    mShowDetails2 = new QPushButton( i18nc( "@action:button", "Show Details" ), topFrame );
    mShowDetails2->setToolTip(
      i18nc( "@info:tooltip", "Hide/Show incidence details" ) );
    mShowDetails2->setWhatsThis(
      i18nc( "@info:whatsthis",
             "Press this button to toggle the incidence details display." ) );
    connect( mShowDetails2, SIGNAL(clicked()), this, SLOT(showIncidence2()) );
    topLayout->addWidget( mShowDetails2, iii, 2 );
    ++iii;
    //
#if 0
    // commented out for now, because the diff code has too many bugs
    mDiffBut = new QPushButton( i18nc( "@action:button", "Show Differences" ), topFrame );
    mDiffBut->setToolTip(
      i18nc( "@info:tooltip", "Show the differences between the two incidences" ) );
    mDiffBut->setWhatsThis(
      i18nc( "@info:whatsthis",
             "Press the \"Show Differences\" button to see the specific "
             "differences between the incidences which are in conflict." ) );
    connect( mDiffBut, SIGNAL(clicked()), this, SLOT(showDiff()) );
    topLayout->addWidget( mDiffBut, iii, 0, 1, 3 );
    ++iii;
#else
    mDiffBut = 0;
#endif
    QGroupBox *groupBox = new QGroupBox( i18nc( "@title:group", "Sync Preferences" ), topFrame );
    QVBoxLayout *groupBoxLayout = new QVBoxLayout;
    mBg = new QButtonGroup( topFrame );
    groupBox->setToolTip( i18nc( "@info:tooltip", "Sync Preferences" ) );
    groupBox->setWhatsThis( i18nc( "@info:whatsthis", "Sync Preferences" ) );
    topLayout->addWidget( groupBox, iii, 0, 1, 3 );
    ++iii;

    QRadioButton *locRad = new QRadioButton(
      i18nc( "@option:radio", "Take local copy on conflict" ) );
    mBg->addButton( locRad, KIncidenceChooser::local );
    locRad->setToolTip(
      i18nc( "@info:tooltip", "Take local copy of the incidence on conflicts" ) );
    locRad->setWhatsThis(
      i18nc( "@info:whatsthis",
             "When a conflict is detected between a local copy of an incidence "
             "and a remote incidence on the server, this option enforces using "
             "the local copy." ) );
    groupBoxLayout->addWidget( locRad );

    QRadioButton *remRad = new QRadioButton(
      i18nc( "@option:radio", "Take remote copy on conflict" ) );
    mBg->addButton( remRad, KIncidenceChooser::remote );
    remRad->setToolTip(
      i18nc( "@info:tooltip", "Take remote copy of the incidence on conflicts" ) );
    remRad->setWhatsThis(
      i18nc( "@info:whatsthis",
             "When a conflict is detected between a local copy of an incidence "
             "and a remote incidence on the server, this option enforces using "
             "the remote copy." ) );
    groupBoxLayout->addWidget( remRad );

    QRadioButton *newRad = new QRadioButton(
      i18nc( "@option:radio", "Take newest incidence on conflict" ) );
    mBg->addButton( newRad, KIncidenceChooser::newest );
    newRad->setToolTip(
      i18nc( "@info:tooltip", "Take newest version of the incidence on conflicts" ) );
    newRad->setWhatsThis(
      i18nc( "@info:whatsthis",
             "When a conflict is detected between a local copy of an incidence "
             "and a remote incidence on the server, this option enforces using "
             "the newest version available." ) );
    groupBoxLayout->addWidget( newRad );

    QRadioButton *askRad = new QRadioButton(
      i18nc( "@option:radio", "Ask for every conflict" ) );
    mBg->addButton( askRad, KIncidenceChooser::ask );
    askRad->setToolTip(
      i18nc( "@info:tooltip", "Ask for every incidence conflict" ) );
    askRad->setWhatsThis(
      i18nc( "@info:whatsthis",
             "When a conflict is detected between a local copy of an incidence "
             "and a remote incidence on the server, this option says to ask "
             "the user which version they want to keep." ) );
    groupBoxLayout->addWidget( askRad );

    QRadioButton *bothRad = new QRadioButton(
      i18nc( "@option:radio", "Take both on conflict" ) );
    mBg->addButton( bothRad, KIncidenceChooser::both );
    bothRad->setToolTip(
      i18nc( "@info:tooltip", "Take both incidences on conflict" ) );
    bothRad->setWhatsThis(
      i18nc( "@info:whatsthis",
             "When a conflict is detected between a local copy of an incidence "
             "and a remote incidence on the server, this option says to keep "
             "both versions of the incidence." ) );
    groupBoxLayout->addWidget( bothRad );

    mBg->button( chooseMode )->setChecked( true );
    groupBox->setLayout( groupBoxLayout );

    QPushButton *applyBut = new QPushButton(
      i18nc( "@action:button", "Apply preference to all conflicts of this sync" ), topFrame );
    connect( applyBut, SIGNAL(clicked()), this, SLOT(setSyncMode()) );
    applyBut->setToolTip(
      i18nc( "@info:tooltip",
             "Apply the preference to all conflicts that may occur during the sync" ) );
    applyBut->setWhatsThis(
      i18nc( "@info:whatsthis",
             "Press this button to apply the selected preference to all "
             "future conflicts that might occur during this sync." ) );
    topLayout->addWidget( applyBut, iii, 0, 1, 3 );

    mTbL = 0;
    mTbN =  0;
    mDisplayDiff = 0;
    mSelIncidence = 0;
}

KIncidenceChooser::~KIncidenceChooser()
{
  if ( mTbL ) {
    delete mTbL;
  }
  if ( mTbN ) {
    delete mTbN;
  }
  if ( mDisplayDiff ) {
    delete mDisplayDiff;
    delete diff;
  }
}

void KIncidenceChooser::setIncidence( Incidence *local, Incidence *remote )
{
  mInc1 = local;
  mInc2 = remote;
  setLabels();

}
Incidence *KIncidenceChooser::getIncidence( )
{
  Incidence *retval = mSelIncidence;
  if ( chooseMode == KIncidenceChooser::local ) {
    retval = mInc1;
  } else if ( chooseMode == KIncidenceChooser::remote ) {
    retval = mInc2;
  } else if ( chooseMode == KIncidenceChooser::both ) {
    retval = 0;
  } else if ( chooseMode == KIncidenceChooser::newest ) {
    if ( mInc1->lastModified() == mInc2->lastModified() ) {
      retval = 0;
    }
    if ( mInc1->lastModified() >  mInc2->lastModified() ) {
      retval =  mInc1;
    } else {
      retval = mInc2;
    }
  }
  return retval;
}

void KIncidenceChooser::setSyncMode()
{
  chooseMode = mBg->checkedId();
  if ( chooseMode != KIncidenceChooser::ask ) {
    KDialog::accept();
  }
}

void KIncidenceChooser::useGlobalMode()
{
  if ( chooseMode != KIncidenceChooser::ask ) {
    KDialog::reject();
  }
}

void KIncidenceChooser::setLabels()
{
  Incidence *inc = mInc1;
  QLabel *des = mInc1lab;
  QLabel *sum = mInc1Sumlab;

  if ( inc->type() == "Event" ) {
    des->setText( i18nc( "@label", "Local Event" ) );
    sum->setText( inc->summary().left( 30 ) );
    if ( mDiffBut ) {
      mDiffBut->setEnabled( true );
    }
  } else if ( inc->type() == "Todo" ) {
    des->setText( i18nc( "@label", "Local Todo" ) );
    sum->setText( inc->summary().left( 30 ) );
    if ( mDiffBut ) {
      mDiffBut->setEnabled( true );
    }
  } else if ( inc->type() == "Journal" ) {
    des->setText( i18nc( "@label", "Local Journal" ) );
    sum->setText( inc->description().left( 30 ) );
    if ( mDiffBut ) {
      mDiffBut->setEnabled( false );
    }
  }
  mMod1lab->setText( KGlobal::locale()->formatDateTime( inc->lastModified().dateTime() ) );
  inc = mInc2;
  des = mInc2lab;
  sum = mInc2Sumlab;
  if ( inc->type() == "Event" ) {
    des->setText( i18nc( "@label", "New Event" ) );
    sum->setText( inc->summary().left( 30 ) );
  } else if ( inc->type() == "Todo" ) {
    des->setText( i18nc( "@label", "New Todo" ) );
    sum->setText( inc->summary().left( 30 ) );
  } else if ( inc->type() == "Journal" ) {
    des->setText( i18nc( "@label", "New Journal" ) );
    sum->setText( inc->description().left( 30 ) );

  }
  mMod2lab->setText( KGlobal::locale()->formatDateTime( inc->lastModified().dateTime() ) );
}

void KIncidenceChooser::showIncidence1()
{
  if ( mTbL ) {
    if ( mTbL->isVisible() ) {
      mShowDetails1->setText( i18nc( "@action:button", "Show Details" ) );
      mTbL->hide();
    } else {
      mShowDetails1->setText( i18nc( "@action:button", "Hide Details" ) );
      mTbL->show();
      mTbL->raise();
    }
    return;
  }
  mTbL = new KDialog( this );
  mTbL->setCaption( mInc1lab->text() );
  mTbL->setModal( false );
  mTbL->setButtons( Ok );
  connect( mTbL, SIGNAL(okClicked()), this, SLOT(detailsDialogClosed()) );
  KTextBrowser *textBrowser = new KTextBrowser( mTbL );
  mTbL->setMainWidget( textBrowser );
  textBrowser->setHtml( IncidenceFormatter::extensiveDisplayStr( 0, mInc1 ) );
  textBrowser->setToolTip( i18nc( "@info:tooltip", "Incidence details" ) );
  textBrowser->setWhatsThis( i18nc( "@info:whatsthis",
                                    "This area shows the incidence details" ) );
  mTbL->setMinimumSize( 400, 400 );
  mShowDetails1->setText( i18nc( "@action:button", "Hide Details" ) );
  mTbL->show();
  mTbL->raise();
}

void KIncidenceChooser::detailsDialogClosed()
{
  KDialog *dialog = static_cast<KDialog *>( const_cast<QObject *>( sender() ) );
  if ( dialog == mTbL ) {
    mShowDetails1->setText( i18nc( "@action:button", "Show details..." ) );
  } else {
    mShowDetails2->setText( i18nc( "@action:button", "Show details..." ) );
  }
}

void KIncidenceChooser::showDiff()
{
  if ( mDisplayDiff ) {
    mDisplayDiff->show();
    mDisplayDiff->raise();
    return;
  }
  mDisplayDiff = new KPIM::HTMLDiffAlgoDisplay( this );
  if ( mInc1->summary().left( 20 ) != mInc2->summary().left( 20 ) ) {
    mDisplayDiff->setWindowTitle(
      i18nc( "@title:window",
             "Differences of %1 and %2",
             mInc1->summary().left( 20 ), mInc2->summary().left( 20 ) ) );
  } else {
    mDisplayDiff->setWindowTitle(
      i18nc( "@title:window",
             "Differences of %1", mInc1->summary().left( 20 ) ) );
  }

  diff = new KPIM::CalendarDiffAlgo( mInc1, mInc2 );
  diff->setLeftSourceTitle( i18nc( "@title:column", "Local incidence" ) );
  diff->setRightSourceTitle( i18nc( "@title:column", "Remote incidence" ) );
  diff->addDisplay( mDisplayDiff );
  diff->run();
  mDisplayDiff->show();
  mDisplayDiff->raise();
}

void KIncidenceChooser::showIncidence2()
{
  if ( mTbN ) {
    if ( mTbN->isVisible() ) {
      mShowDetails2->setText( i18nc( "@label", "Show Details" ) );
      mTbN->hide();
    } else {
      mShowDetails2->setText( i18nc( "@label", "Hide Details" ) );
      mTbN->show();
      mTbN->raise();
    }
    return;
  }
  mTbN = new KDialog( this );
  mTbN->setCaption( mInc2lab->text() );
  mTbN->setModal( false );
  mTbN->setButtons( Ok );
  connect( mTbN, SIGNAL(okClicked()), this, SLOT(detailsDialogClosed()) );
  KTextBrowser *textBrowser = new KTextBrowser( mTbN );
  mTbN->setMainWidget( textBrowser );
  textBrowser->setHtml( IncidenceFormatter::extensiveDisplayStr( 0, mInc2 ) );
  textBrowser->setToolTip( i18nc( "@info:tooltip", "Incidence details" ) );
  textBrowser->setWhatsThis( i18nc( "@info:whatsthis",
                                    "This area shows the incidence details" ) );
  mTbN->setMinimumSize( 400, 400 );
  mShowDetails2->setText( i18nc( "@label", "Hide Details" ) );
  mTbN->show();
  mTbN->raise();
}

void KIncidenceChooser::takeIncidence1()
{
  mSelIncidence = mInc1;
  KDialog::accept();
}

void KIncidenceChooser::takeIncidence2()
{
  mSelIncidence = mInc2;
  KDialog::accept();
}

void KIncidenceChooser::takeBoth()
{
  mSelIncidence = 0;
  KDialog::accept();
}

#include "kincidencechooser.moc"
