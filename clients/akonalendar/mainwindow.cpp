/*
    This file is part of Akonadi.

    Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "mainwindow.h"
#include "mainwidget.h"

#include <QtGui/QGridLayout>
#include <QtGui/QToolBar>
#include <QtGui/QDialog>

#include <KCModuleLoader>

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
{
    QToolBar *toolBar = new QToolBar( QLatin1String( "Main toolbar" ), this );
    toolBar->addAction( "Configure ...", this, SLOT( configure() ) );
    addToolBar( toolBar );

    setCentralWidget( new MainWidget( this ) );
    resize( 700, 500 );
}

void MainWindow::configure()
{
    QDialog *configDialog = new QDialog( this );
    QGridLayout *layout = new QGridLayout();
    QWidget *kcmWidget = KCModuleLoader::loadModule(  "kcm_akonadi_resources",
                                                    KCModuleLoader::Inline, configDialog, QStringList( "text/calendar" ) );
    layout->addWidget( kcmWidget );
    configDialog->setLayout( layout );
    configDialog->show();
}
