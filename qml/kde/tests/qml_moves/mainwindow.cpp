/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "mainwindow.h"

#include <QHBoxLayout>
#include <QApplication>
#include <QTreeView>
#include <QTimer>
#include <QSplitter>
#include <QDeclarativeView>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

#include "dynamictreemodel.h"

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags f )
  : QWidget(parent, f)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  QSplitter *splitter = new QSplitter;
  layout->addWidget(splitter);

  m_treeModel = new DynamicTreeModel(this);
  ModelInsertCommand *insert = new ModelInsertCommand(m_treeModel, this);
  insert->setStartRow(0);
  insert->interpret(
    "- 1"
    "- 1"
    "- 1"
    "- 1"
    "- 1"
  );
  insert->doCommand();

  QTreeView *view = new QTreeView(splitter);
  view->setModel(m_treeModel);

  m_declarativeView = new QDeclarativeView(splitter);

  QDeclarativeContext *context = m_declarativeView->engine()->rootContext();

  context->setContextProperty( "_model", m_treeModel );

  context->setContextProperty( "application", QVariant::fromValue( static_cast<QObject*>( this ) ) );

  m_declarativeView->setResizeMode( QDeclarativeView::SizeRootObjectToView );
  m_declarativeView->setSource( QUrl( "./mainview.qml" ) );

  splitter->setSizes(QList<int>() << 1 << 1);
  QTimer::singleShot(2000, this, SLOT(doMove()));

}

void MainWindow::doMove()
{
  qDebug() << "MOV";
  ModelMoveCommand *command = new ModelMoveCommand(m_treeModel, this);
  command->setStartRow(0);
  command->setEndRow(0);
  command->setDestRow(2);
  command->doCommand();
}

//#include "mainwindow.moc"
