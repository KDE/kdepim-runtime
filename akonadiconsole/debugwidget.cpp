/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include "debugwidget.h"

#include "tracernotificationinterface.h"
#include "connectionpage.h"

#include <akonadi/control.h>

#include <KLocale>

#include <QtGui/QPushButton>
#include <QtGui/QSplitter>
#include <QtGui/QTabWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>

using org::freedesktop::Akonadi::DebugInterface;

DebugWidget::DebugWidget( QWidget *parent )
  : QWidget( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  mDebugInterface = new DebugInterface( "org.freedesktop.Akonadi", "/debug", QDBusConnection::sessionBus(), this );
  QCheckBox *cb = new QCheckBox( i18n("Enable debugger"), this );
  cb->setChecked( mDebugInterface->isValid() && mDebugInterface->tracer().value() == QLatin1String( "dbus" ) );
  connect( cb, SIGNAL(toggled(bool)), SLOT(enableDebugger(bool)) );
  layout->addWidget( cb );

  QSplitter *splitter = new QSplitter( Qt::Vertical, this );
  splitter->setObjectName( "debugSplitter" );
  layout->addWidget( splitter );

  mConnectionPages = new QTabWidget( splitter );

  mGeneralView = new QTextEdit( splitter );
  mGeneralView->setReadOnly( true );

  ConnectionPage *page = new ConnectionPage( "All" );
  page->showAllConnections( true );
  mConnectionPages->addTab( page, "All" );

  org::freedesktop::Akonadi::TracerNotification *iface = new org::freedesktop::Akonadi::TracerNotification( QString(), "/tracing/notifications", QDBusConnection::sessionBus(), this );

  connect( iface, SIGNAL( connectionStarted( const QString&, const QString& ) ),
           this, SLOT( connectionStarted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( connectionEnded( const QString&, const QString& ) ),
           this, SLOT( connectionEnded( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( signalEmitted( const QString&, const QString& ) ),
           this, SLOT( signalEmitted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( warningEmitted( const QString&, const QString& ) ),
           this, SLOT( warningEmitted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( errorEmitted( const QString&, const QString& ) ),
           this, SLOT( errorEmitted( const QString&, const QString& ) ) );

  // in case we started listening when the connection is already ongoing
  connect( iface, SIGNAL( connectionDataInput( const QString&, const QString& ) ),
           this, SLOT( connectionStarted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( connectionDataOutput( const QString&, const QString& ) ),
           this, SLOT( connectionStarted( const QString&, const QString& ) ) );

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  layout->addLayout( buttonLayout );

  QPushButton *clearAllButton = new QPushButton( "Clear All", this );
  QPushButton *clearGeneralButton = new QPushButton( "Clear General", this );

  buttonLayout->addWidget( clearAllButton );
  buttonLayout->addWidget( clearGeneralButton );

  connect( clearAllButton, SIGNAL( clicked() ), page, SLOT( clear() ) );
  connect( clearGeneralButton, SIGNAL( clicked() ), mGeneralView, SLOT( clear() ) );

  Akonadi::Control::widgetNeedsAkonadi( this );
}

void DebugWidget::connectionStarted( const QString &identifier, const QString &msg )
{
  Q_UNUSED( msg );
  if ( mPageHash.contains( identifier ) )
    return;

  ConnectionPage *page = new ConnectionPage( identifier );
  mConnectionPages->addTab( page, identifier );

  mPageHash.insert( identifier, page );
}

void DebugWidget::connectionEnded( const QString &identifier, const QString& )
{
  if ( !mPageHash.contains( identifier ) )
    return;

  QWidget *widget = mPageHash[ identifier ];

  mConnectionPages->removeTab( mConnectionPages->indexOf( widget ) );

  mPageHash.remove( identifier );
  delete widget;
}

void DebugWidget::signalEmitted( const QString &signalName, const QString &msg )
{
  mGeneralView->append( QString( "<font color=\"green\">%1 ( %2 )</font>" ).arg( signalName, msg ) );
}

void DebugWidget::warningEmitted( const QString &componentName, const QString &msg )
{
  mGeneralView->append( QString( "<font color=\"blue\">%1: %2</font>" ).arg( componentName, msg ) );
}

void DebugWidget::errorEmitted( const QString &componentName, const QString &msg )
{
  mGeneralView->append( QString( "<font color=\"red\">%1: %2</font>" ).arg( componentName, msg ) );
}

void DebugWidget::enableDebugger(bool enable)
{
  mDebugInterface->setTracer( enable ? QLatin1String( "dbus" ) : QLatin1String( "null" ) );
}

#include "debugwidget.moc"
