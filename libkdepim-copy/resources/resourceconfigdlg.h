/*
    This file is part of libkdepim.
    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>

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

#ifndef KPIM_RESOURCECONFIGDLG_H
#define KPIM_RESOURCECONFIGDLG_H

#include <qcheckbox.h>

#include <kbuttonbox.h>
#include <kdialog.h>
#include <kconfig.h>
#include <klineedit.h>

#include "resourceconfigwidget.h"

class ResourceConfigDlg : KDialog
{
  Q_OBJECT

public:
  ResourceConfigDlg( QWidget *parent, const QString& resourceType, 
	  const QString& type, KConfig *config, const char *name = 0);

  bool readOnly();
  bool fast();
  QString resourceName();

public slots:
  int exec();

  void setReadOnly( bool value );
  void setFast( bool value );
  void setResourceName( const QString &name );

protected slots:
  void accept();

private:
  KPIM::ResourceConfigWidget *mConfigWidget;
  KConfig *mConfig;

  KButtonBox *mButtonBox;
  KLineEdit *mName;
  QCheckBox *mReadOnly;
  QCheckBox *mFast;
};

#endif
