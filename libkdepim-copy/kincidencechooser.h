/*
    This file is part of libkdepim.

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
#ifndef _KINCIDENCECHOOSER_H
#define _KINCIDENCECHOOSER_H


#include <kdialogbase.h>
#include <qptrlist.h>
#include <qmutex.h>
#include <kdepimmacros.h>

#include <libkcal/incidence.h>
#include "htmldiffalgodisplay.h"
#include "calendardiffalgo.h"


class QRadioButton;
class QButtonGroup;
class QVBox;
class QStringList;
class QTextBrowser;


/** Dialog to change the korganizer configuration.
  */

class KDE_EXPORT KIncidenceChooser : public KDialog
{
    Q_OBJECT
public:
    enum mode { local, remote, newest, ask, both };
    /** Initialize dialog and pages */
    KIncidenceChooser(QWidget *parent=0,char *name=0);
    ~KIncidenceChooser();
    //void setChooseText( QString );
    void setIncidence( KCal::Incidence*,KCal::Incidence*);
    KCal::Incidence* getIncidence();
    static int chooseMode;

    public slots:
    void useGlobalMode();

    protected slots:
    void showIncidence1();
    void showIncidence2();
    void showDiff();
    void takeIncidence1();
    void takeIncidence2();
    void takeBoth();
    void setLabels();
    void setSyncMode();

 protected:
 private:
    KPIM::HTMLDiffAlgoDisplay* mDisplayDiff;
    KPIM::CalendarDiffAlgo* diff;
    QTextBrowser* mTbL, *mTbN;
    KCal::Incidence* choosedIncidence;
    KCal::Incidence* mInc1, *mInc2;
    QButtonGroup *mBg;
    QPushButton *diffBut,*showDetails1,*showDetails2;
    QLabel* mInc1lab, *mInc2lab,* mInc1Sumlab, *mInc2Sumlab,*mMod1lab,*mMod2lab;

};

#endif
