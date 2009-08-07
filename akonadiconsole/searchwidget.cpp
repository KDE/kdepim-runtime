/*
    This file is part of Akonadi.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "searchwidget.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsearchjob.h>
#include <kmessagebox.h>

#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QPushButton>
#include <QtGui/QStringListModel>
#include <QtGui/QTextBrowser>
#include <QtGui/QTextEdit>

#include <QtCore/QDebug>

SearchWidget::SearchWidget( QWidget *parent )
  : QWidget( parent )
{
  QGridLayout *layout = new QGridLayout( this );

  mQueryCombo = new QComboBox;
  mQueryWidget = new QTextEdit;
  mResultView = new QListView;
  mItemView = new QTextBrowser;
  QPushButton *button = new QPushButton( "Search" );

  layout->addWidget( new QLabel( "SPARQL Query:" ), 0, 0 );
  layout->addWidget( mQueryCombo, 0, 1, Qt::AlignRight );

  layout->addWidget( mQueryWidget, 1, 0, 1, 2 );

  layout->addWidget( new QLabel( "Matching Item UIDs:" ), 2, 0 );
  layout->addWidget( new QLabel( "View:" ), 2, 1 );

  layout->addWidget( mResultView, 3, 0, 1, 1 );
  layout->addWidget( mItemView, 3, 1, 1, 1 );

  layout->addWidget( button, 4, 1, Qt::AlignRight );

  mQueryCombo->addItem( "Empty" );
  mQueryCombo->addItem( "Contacts by email address" );

  connect( button, SIGNAL( clicked() ), this, SLOT( search() ) );
  connect( mQueryCombo, SIGNAL( activated( int ) ), this, SLOT( querySelected( int ) ) );
  connect( mResultView, SIGNAL( activated( const QModelIndex& ) ), this, SLOT( fetchItem( const QModelIndex& ) ) );

  mResultModel = new QStringListModel( this );
  mResultView->setModel( mResultModel );
}

SearchWidget::~SearchWidget()
{
}

void SearchWidget::search()
{
  Akonadi::ItemSearchJob *job = new Akonadi::ItemSearchJob( mQueryWidget->toPlainText() );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( searchFinished( KJob* ) ) );
  job->start();
}

void SearchWidget::searchFinished( KJob *job )
{
  mResultModel->setStringList( QStringList() );
  mItemView->clear();

  if ( job->error() ) {
    KMessageBox::error( this, job->errorString() );
    return;
  }

  QStringList uidList;
  Akonadi::ItemSearchJob *searchJob = qobject_cast<Akonadi::ItemSearchJob*>( job );
  const Akonadi::Item::List items = searchJob->items();
  foreach ( const Akonadi::Item &item, items ) {
    uidList << QString::number( item.id() );
  }

  mResultModel->setStringList( uidList );
}

void SearchWidget::querySelected( int index )
{
  if ( index == 0 ) {
    mQueryWidget->clear();
  } else if ( index == 1 ) {
    mQueryWidget->setPlainText( ""
                                "SELECT ?person WHERE {\n"
                                "  ?person <http://www.semanticdesktop.org/ontologies/2007/03/22/nco#hasEmailAddress> ?email .\n"
                                "  ?email <http://www.semanticdesktop.org/ontologies/2007/03/22/nco#emailAddress> \"tokoe@kde.org\"^^<http://www.w3.org/2001/XMLSchema#string> .\n"
                                " }\n"
                              );
  }
}

void SearchWidget::fetchItem( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const QString uid = index.data( Qt::DisplayRole ).toString();
  mFetchJob = new Akonadi::ItemFetchJob( Akonadi::Item( uid.toLongLong() ) );
  mFetchJob->fetchScope().fetchFullPayload();
  connect( mFetchJob, SIGNAL( result( KJob* ) ), this, SLOT( itemFetched( KJob* ) ) );
  mFetchJob->start();
}

void SearchWidget::itemFetched( KJob *job )
{
  mItemView->clear();

  if ( job->error() ) {
    KMessageBox::error( this, "Error on fetching item" );
    return;
  }

  const Akonadi::Item item = mFetchJob->items().first();
  mItemView->setPlainText( QString::fromUtf8( item.payloadData() ) );

  mFetchJob = 0;
}

#include "searchwidget.moc"
