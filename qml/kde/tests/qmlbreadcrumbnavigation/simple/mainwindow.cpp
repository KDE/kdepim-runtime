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
#include <QSplitter>
#include <QDeclarativeView>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QTimer>

#include <QStandardItemModel>

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags f )
  : QWidget(parent, f)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  QSplitter *splitter = new QSplitter;
  layout->addWidget(splitter);

  QTreeView *treeView = new QTreeView(splitter);

  m_model = new QStandardItemModel(this);
  treeView->setModel(m_model);

  appendRow();
  appendRow();

  m_declarativeView = new QDeclarativeView(splitter);

  QDeclarativeContext *context = m_declarativeView->engine()->rootContext();

  context->setContextProperty( "myModel", QVariant::fromValue( static_cast<QObject*>( m_model ) ) );

  m_declarativeView->setResizeMode( QDeclarativeView::SizeRootObjectToView );
  m_declarativeView->setSource( QUrl( "./mainview.qml" ) );

  splitter->setSizes(QList<int>() << 1 << 1);

//   QTimer::singleShot(4000, this, SLOT(removeBottomRow()));
  QTimer::singleShot(8000, this, SLOT(prependNewRow()));

}

static int sCount = 1;

void MainWindow::appendRow()
{
  QStandardItem *item = new QStandardItem(QString::fromLatin1("Item %1").arg(sCount++));
  m_model->appendRow(item);
}

void MainWindow::prependNewRow()
{
  QStandardItem *item = new QStandardItem(QString::fromLatin1("Item %1").arg(sCount++));
  m_model->insertRow(0, item);
}

void MainWindow::removeTopRow()
{
  m_model->removeRow(0);
}

void MainWindow::removeBottomRow()
{
  m_model->removeRow(1);
}


#include "mainwindow.moc"
