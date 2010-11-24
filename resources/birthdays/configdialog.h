/*
    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <kdeversion.h>
#if KDE_IS_VERSION( 4, 5, 74 )
#include "ui_configdialog.h"
#else
#include "ui_configdialog-45.h"
#endif

#include <KDialog>

class KConfigDialogManager;

class ConfigDialog : public KDialog
{
  Q_OBJECT
  public:
    ConfigDialog( QWidget *parent = 0 );
    ~ConfigDialog() {};

  private slots:
    void save();

  private:
    Ui::ConfigDialog ui;
    KConfigDialogManager* mManager;
};

#endif
