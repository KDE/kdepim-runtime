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

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef _KINCIDENCECHOOSER_H
#define _KINCIDENCECHOOSER_H

#include "kdepim_export.h"
#include "htmldiffalgodisplay.h"
#include "calendardiffalgo.h"

#include <kdialog.h>

#include <QMutex>
#include <QLabel>

namespace KCal {
  class Incidence;
}
class Q3ButtonGroup;
class KDialog;

namespace KPIM {

/**
  @brief
  Dialog to change the korganizer configuration.
*/

class KDEPIM_EXPORT KIncidenceChooser : public KDialog
{
  Q_OBJECT
  public:
    enum mode {
      local,
      remote,
      newest,
      ask,
      both
    };

    /** Initialize dialog and pages */
    KIncidenceChooser( QWidget *parent=0 );
    ~KIncidenceChooser();
    void setIncidence( KCal::Incidence *, KCal::Incidence * );
    KCal::Incidence *getIncidence();
    static int chooseMode;

  public Q_SLOTS:
    void useGlobalMode();

  protected Q_SLOTS:
    void showIncidence1();
    void showIncidence2();
    void showDiff();
    void takeIncidence1();
    void takeIncidence2();
    void takeBoth();
    void setLabels();
    void setSyncMode();
    void detailsDialogClosed();

  private:
    KPIM::HTMLDiffAlgoDisplay *mDisplayDiff;
    KPIM::CalendarDiffAlgo *diff;
    KDialog *mTbL, *mTbN;
    KCal::Incidence *choosedIncidence;
    KCal::Incidence *mInc1, *mInc2;
    Q3ButtonGroup *mBg;
    QPushButton *diffBut,*showDetails1,*showDetails2;
    QLabel *mInc1lab, *mInc2lab,* mInc1Sumlab, *mInc2Sumlab,*mMod1lab,*mMod2lab;
};

}

#endif
