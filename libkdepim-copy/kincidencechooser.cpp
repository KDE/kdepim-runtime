/*
    This file is part of KOrganizer.
    Copyright (c) 2004 Lutz Rogowski <rogowski@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <qlayout.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qtextbrowser.h>
#include <qapplication.h>


#include <klocale.h>
#include <kglobal.h>

#include "kincidencechooser.h"
#include "libkcal/incidence.h"
#include "libkcal/incidenceformatter.h"

int KIncidenceChooser::chooseMode = KIncidenceChooser::ask ;

KIncidenceChooser::KIncidenceChooser(QWidget *parent, char *name) :
    KDialog(parent,name,true)
{
    KDialog *topFrame = this;
    QGridLayout *topLayout = new QGridLayout(topFrame,5,3);
    int iii = 0;
    setCaption( i18n("Conflict Detected"));
    QLabel * lab;
    lab = new QLabel( i18n(
                        "<qt>A conflict was detected. This probably means someone edited the same entry on the server while you changed it locally."
                        "<br/>NOTE: You have to check mail again to apply your changes to the server.</qt>"), topFrame);
    topLayout->addMultiCellWidget(lab, iii,iii,0,2);
    ++iii;
    QHBox * b_box = new QHBox( topFrame );
    topLayout->addMultiCellWidget(b_box, iii,iii,0,2);
    ++iii;
    QPushButton* button = new QPushButton( i18n("Take Local"), b_box );
    connect ( button, SIGNAL( clicked()), this, SLOT (takeIncidence1() ) );
    button = new QPushButton( i18n("Take New"), b_box );
    connect ( button, SIGNAL( clicked()), this, SLOT (takeIncidence2() ) );
    button = new QPushButton( i18n("Take Both"), b_box );
    connect ( button, SIGNAL( clicked()), this, SLOT (takeBoth() ) );
    topLayout->setSpacing(spacingHint());
    topLayout->setMargin(marginHint());
    // text is not translated, because text has to be set later
    mInc1lab = new QLabel ( i18n("Local incidence"), topFrame);
    topLayout->addWidget(mInc1lab ,iii,0);
    mInc1Sumlab = new QLabel ( i18n("Local incidence summary"), topFrame);
    topLayout->addMultiCellWidget(mInc1Sumlab, iii,iii,1,2);
    ++iii;
    topLayout->addWidget( new QLabel ( i18n("Last modified:"), topFrame) ,iii,0);
    mMod1lab = new QLabel ( "Set Last modified", topFrame);
    topLayout->addWidget(mMod1lab,iii,1);
    showDetails1 = new QPushButton( i18n("Show Details..."),topFrame );
    connect ( showDetails1, SIGNAL( clicked()), this, SLOT (showIncidence1() ) );
    topLayout->addWidget(showDetails1,iii,2);
    ++iii;

    mInc2lab = new QLabel ( "Local incidence", topFrame);
    topLayout->addWidget(mInc2lab,iii,0);
    mInc2Sumlab = new QLabel ( "Local incidence summary", topFrame);
    topLayout->addMultiCellWidget(mInc2Sumlab, iii,iii,1,2);
    ++iii;
    topLayout->addWidget( new QLabel ( i18n("Last modified:"), topFrame) ,iii,0);
    mMod2lab = new QLabel ( "Set Last modified", topFrame);
    topLayout->addWidget(mMod2lab,iii,1);
    showDetails2 = new QPushButton( i18n("Show Details..."), topFrame);
    connect ( showDetails2, SIGNAL( clicked()), this, SLOT (showIncidence2() ) );
    topLayout->addWidget(showDetails2,iii,2);
    ++iii;
    //
#if 0
    // commented out for now, because the diff code has too many bugs
    diffBut = new QPushButton( i18n("Show Differences"), topFrame );
    connect ( diffBut, SIGNAL( clicked()), this, SLOT ( showDiff() ) );
    topLayout->addMultiCellWidget(diffBut, iii,iii,0,2);
    ++iii;
#else
    diffBut = 0;
#endif
    mBg = new QButtonGroup ( 1,  Qt::Horizontal, i18n("Sync Preferences"), topFrame);
    topLayout->addMultiCellWidget(mBg, iii,iii,0,2);
    ++iii;
    mBg->insert( new QRadioButton ( i18n("Take local entry on conflict"), mBg ), KIncidenceChooser::local);
    mBg->insert( new QRadioButton ( i18n("Take new (remote) entry on conflict"), mBg ),  KIncidenceChooser::remote);
    mBg->insert( new QRadioButton ( i18n("Take newest entry on conflict"), mBg ), KIncidenceChooser::newest );
    mBg->insert( new QRadioButton ( i18n("Ask for every entry on conflict"), mBg ),KIncidenceChooser::ask );
    mBg->insert( new QRadioButton ( i18n("Take both on conflict"), mBg ), KIncidenceChooser::both );
    mBg->setButton ( chooseMode );
    mTbL = 0;
    mTbN =  0;
    mDisplayDiff = 0;
    choosedIncidence = 0;
    button = new QPushButton( i18n("Apply This to All Conflicts of This Sync"), topFrame );
    connect ( button, SIGNAL( clicked()), this, SLOT ( setSyncMode() ) );
    topLayout->addMultiCellWidget(button, iii,iii,0,2);
}

KIncidenceChooser::~KIncidenceChooser()
{
    if ( mTbL ) delete mTbL;
    if ( mTbN ) delete mTbN;
    if ( mDisplayDiff ) {
        delete mDisplayDiff;
        delete diff;
    }
}

void KIncidenceChooser::setIncidence( KCal::Incidence* local ,KCal::Incidence* remote )
{
    mInc1 = local;
    mInc2 = remote;
    setLabels();

}
KCal::Incidence* KIncidenceChooser::getIncidence( )
{

    KCal::Incidence* retval = choosedIncidence;
    if ( chooseMode == KIncidenceChooser::local )
        retval = mInc1;
    else  if ( chooseMode == KIncidenceChooser::remote )
        retval = mInc2;
    else  if ( chooseMode == KIncidenceChooser::both ) {
        retval = 0;
    }
    else  if ( chooseMode == KIncidenceChooser::newest ) {
        if ( mInc1->lastModified() == mInc2->lastModified())
            retval = 0;
        if ( mInc1->lastModified() >  mInc2->lastModified() )
            retval =  mInc1;
        else
            retval = mInc2;
    }
    return retval;
}

void KIncidenceChooser::setSyncMode()
{
    chooseMode = mBg->selectedId ();
    if ( chooseMode != KIncidenceChooser::ask )
        QDialog::accept();

}
void KIncidenceChooser::useGlobalMode()
{
 if ( chooseMode != KIncidenceChooser::ask )
        QDialog::reject();
}
void KIncidenceChooser::setLabels()
{
  KCal::Incidence* inc = mInc1;
    QLabel* des = mInc1lab;
    QLabel * sum = mInc1Sumlab;


    if ( inc->type() == "Event" ) {
        des->setText( i18n( "Local Event") );
        sum->setText( inc->summary().left( 30 ));
        if ( diffBut )
            diffBut->setEnabled( true );
    }
    else if ( inc->type() == "Todo" ) {
        des->setText( i18n( "Local Todo") );
        sum->setText( inc->summary().left( 30 ));
        if ( diffBut )
            diffBut->setEnabled( true );

    }
    else if ( inc->type() == "Journal" ) {
        des->setText( i18n( "Local Journal") );
        sum->setText( inc->description().left( 30 ));
        if ( diffBut )
            diffBut->setEnabled( false );
    }
    mMod1lab->setText( KGlobal::locale()->formatDateTime(inc->lastModified() ));
    inc = mInc2;
    des = mInc2lab;
    sum = mInc2Sumlab;
    if ( inc->type() == "Event" ) {
        des->setText( i18n( "New Event") );
        sum->setText( inc->summary().left( 30 ));
    }
    else if ( inc->type() == "Todo" ) {
        des->setText( i18n( "New Todo") );
        sum->setText( inc->summary().left( 30 ));

    }
    else if ( inc->type() == "Journal" ) {
        des->setText( i18n( "New Journal") );
        sum->setText( inc->description().left( 30 ));

    }
    mMod2lab->setText( KGlobal::locale()->formatDateTime(inc->lastModified() ));
}

void KIncidenceChooser::showIncidence1()
{
    if ( mTbL ) {
        if ( mTbL->isVisible() ) {
            showDetails1->setText( i18n("Show details..."));
            mTbL->hide();
        } else {
            showDetails1->setText( i18n("Hide details"));
            mTbL->show();
            mTbL->raise();
        }
        return;
    }
    mTbL =  new QTextBrowser( 0, "incviewer" );
    mTbL->setCaption(mInc1lab->text() );
    mTbL->setText( KCal::IncidenceFormatter::extensiveDisplayString( mInc1 )  );
    mTbL->resize( 300, 300 );
    showDetails1->setText( i18n("Hide details"));
    mTbL->show();
    mTbL->raise();
}
void KIncidenceChooser::showDiff()
{
    if ( mDisplayDiff ) {
        mDisplayDiff->show();
        mDisplayDiff->raise();
        return;
    }
    mDisplayDiff = new KPIM::HTMLDiffAlgoDisplay (0);
    if ( mInc1->summary().left( 20 ) != mInc2->summary().left( 20 ) )
        mDisplayDiff->setCaption( i18n( "Differences of %1 and %2").arg( mInc1->summary().left( 20 ) ).arg( mInc2->summary().left( 20 ) ) );
    else
        mDisplayDiff->setCaption( i18n( "Differences of %1").arg( mInc1->summary().left( 20 ) ) );

    diff = new KPIM::CalendarDiffAlgo( mInc1, mInc2);
    diff->setLeftSourceTitle(  i18n( "Local entry"));
    diff->setRightSourceTitle(i18n( "New (remote) entry") );
    diff->addDisplay( mDisplayDiff );
    diff->run();
    mDisplayDiff->show();
    mDisplayDiff->raise();
}
void KIncidenceChooser::showIncidence2()
{
   if ( mTbN ) {
        if ( mTbN->isVisible() ) {
            showDetails2->setText( i18n("Show details..."));
            mTbN->hide();
        } else {
            showDetails2->setText( i18n("Hide details"));
            mTbN->show();
            mTbN->raise();
        }
        return;
    }
    mTbN =  new QTextBrowser( 0, "incviewer" );
    mTbN->setCaption(mInc2lab->text() );
    mTbN->setText( KCal::IncidenceFormatter::extensiveDisplayString( mInc2 ) );
    mTbN->resize( 300, 300 );
    showDetails2->setText( i18n("Hide details"));
    mTbN->show();
    mTbN->raise();
}
void KIncidenceChooser::takeIncidence1()
{
    choosedIncidence = mInc1;
    QDialog::accept();
}
void KIncidenceChooser::takeIncidence2()
{

    choosedIncidence = mInc2;
    QDialog::accept();
}
void KIncidenceChooser::takeBoth()
{

    choosedIncidence = 0;
    QDialog::accept();
}


#include "kincidencechooser.moc"
