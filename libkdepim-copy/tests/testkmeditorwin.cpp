/* This file is part of the KDE libraries
    Copyright (C) 2007 Laurent Montel <montel@kde.org>

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

#include <kmeditor.h>
#include <KApplication>
#include <kcmdlineargs.h>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <testkmeditorwin.h>
#include "testkmeditorwin.moc"

testKMeditorWindow::testKMeditorWindow()
{
  setCaption("test kmeditor window");
  editor = new KMeditor;
  editor->setAcceptRichText(false);
  setCentralWidget(editor);
  
  QMenu *editMenu = menuBar()->addMenu(tr("Edit"));
  QAction *act = new QAction(tr("bold"), this);
  act->setChecked(true);
  connect(act, SIGNAL(toggled(bool)), editor, SLOT(slotTextBold(bool)));

  editMenu->addAction(act);
}

testKMeditorWindow::~testKMeditorWindow()
{
}


int main( int argc, char **argv )
{
    KCmdLineArgs::init( argc, argv, "testkmeditorwin", "KMeditorTestWin", "kmeditor test win app", "1.0" );
    KApplication app;
    testKMeditorWindow *edit = new testKMeditorWindow();
    edit->show();
    return app.exec();
}

