/*
 * KJots rewrite
 *
 * Copyright 2008  Stephen Kelly <steveire@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "kjotsrewrite.h"

#include "kjotswidget.h"

#include <kapplication.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ksavefile.h>
#include <kstatusbar.h>
#include <kaction.h>
#include <KLocale>


KJotsReWrite::KJotsReWrite() : KXmlGuiWindow()
{

  kjw = new KJotsWidget( this );
  setCentralWidget( kjw );

  KAction *action;
  action = actionCollection()->addAction( "change_theme");
  action->setText( i18n("Change Theme...") );
  connect(action, SIGNAL(triggered()), kjw, SLOT(changeTheme()));

  action = actionCollection()->addAction( "export_selection");
  action->setText( i18n("Export...") );
  connect(action, SIGNAL(triggered()), kjw, SLOT(exportSelection()));

  setupGUI();
}


KJotsReWrite::~KJotsReWrite()
{
}


