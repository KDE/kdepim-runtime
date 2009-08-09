/****************************************************************************** * *
 *
 *  File : widgethighlighter.cpp
 *  Created on Fri 15 May 2009 04:53:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the editoried warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "widgethighlighter.h"

#include <QtCore/QEvent>
#include <QtCore/QTimer>

#include <QtGui/QColor>


namespace Akonadi
{
namespace Filter
{
namespace UI
{
namespace Private
{

WidgetHighlighter::WidgetHighlighter( QWidget * targetAndParent )
  : QObject( targetAndParent )
{
  mHighlightingOn = false;
  mSwitchOffTimer = 0;
  mFlashTimer = new QTimer( this );
  mFlashOn = false;

  connect( mFlashTimer, SIGNAL( timeout() ), this, SLOT( slotFlashWidget() ) );

  QList< WidgetHighlighter * > existingHighlighters = targetAndParent->findChildren< WidgetHighlighter * >();
  if( existingHighlighters.count() > 0 )
  {
    WidgetHighlighter * existing = existingHighlighters.first();
    existing->switchHighlightingOn();
  } else {
    switchHighlightingOn();
  }
}

WidgetHighlighter::~WidgetHighlighter()
{
  switchHighlightingOff();
}

bool WidgetHighlighter::eventFilter( QObject * o, QEvent * e )
{
  switch( e->type() )
  {
    case QEvent::FocusIn:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseButtonPress:
      triggerSwitchHighlightingOff();
    break;
    default:
      // make gcc happy
    break;
  }

  return QObject::eventFilter( o, e );
}

void WidgetHighlighter::triggerSwitchHighlightingOff()
{
  if( mSwitchOffTimer )
    return;
  mSwitchOffTimer = new QTimer( this );
  mSwitchOffTimer->setSingleShot( true );

  connect( mSwitchOffTimer, SIGNAL( timeout() ), this, SLOT( slotSwitchHighlightingOff() ) );

  mSwitchOffTimer->start( 1000 );

  if( mFlashTimer->isActive() )
    mFlashTimer->stop();
}

void WidgetHighlighter::hookEventFilter( QWidget * target )
{
  Q_ASSERT( target );

  target->installEventFilter( this );

  QList< QWidget * > childrenList = target->findChildren< QWidget * >();

  foreach( QWidget * w, childrenList )
    hookEventFilter( w );
}

void WidgetHighlighter::unhookEventFilter( QWidget * target )
{
  Q_ASSERT( target );

  target->removeEventFilter( this );

  QList< QWidget * > childrenList = target->findChildren< QWidget * >();

  foreach( QWidget * w, childrenList )
    unhookEventFilter( w );
}

void WidgetHighlighter::switchHighlightingOn()
{
  if( mHighlightingOn )
    return; // already highlighted

  QWidget * target = dynamic_cast< QWidget * >( parent() );
  if( !target )
  {
    deleteLater();
    return; // hum
  }

  mFlashOn = true;
  mSavedPalette = target->palette();
  mSavedAutoFillBackground = target->autoFillBackground();

  target->setAutoFillBackground( true );

  mHighlightingOn = true;

  hookEventFilter( target );

  highlight( target, 20 );

  mFlashTimer->start( 600 );
}

static inline QColor merge_colors( const QColor &clr1, const QColor &clr2, int percent )
{
  return QColor(
      ( ( clr1.red() * ( 100 - percent ) ) + ( clr2.red() * percent ) ) / 100,
      ( ( clr1.green() * ( 100 - percent ) ) + ( clr2.green() * percent ) ) / 100,
      ( ( clr1.blue() * ( 100 - percent ) ) + ( clr2.blue() * percent ) ) / 100
    );
}

void WidgetHighlighter::highlight( QWidget * target, int percent )
{
  QColor theRed( 80, 0, 0 );

  QPalette pal(
      merge_colors( mSavedPalette.color( QPalette::WindowText ), theRed, percent),
      merge_colors( mSavedPalette.color( QPalette::Button ), theRed, percent ),
      merge_colors( mSavedPalette.color( QPalette::Light ), theRed, percent ),
      merge_colors( mSavedPalette.color( QPalette::Dark ), theRed, percent ),
      merge_colors( mSavedPalette.color( QPalette::Mid ), theRed, percent ),
      merge_colors( mSavedPalette.color( QPalette::Text ), theRed, percent ),
      merge_colors( mSavedPalette.color( QPalette::BrightText ), theRed, percent ),
      merge_colors( mSavedPalette.color( QPalette::Base ), theRed, percent ),
      merge_colors( mSavedPalette.color( QPalette::Window ), theRed, percent )
    );

  target->setPalette( pal );  
}

void WidgetHighlighter::slotFlashWidget()
{
  QWidget * target = dynamic_cast< QWidget * >( parent() );
  if( !target )
    return;

  if( !mHighlightingOn )
    return;

  highlight( target, mFlashOn ? 50 : 20 );
  mFlashOn = !mFlashOn;
}

void WidgetHighlighter::switchHighlightingOff()
{
  if( mSwitchOffTimer )
  {
    delete mSwitchOffTimer;
    mSwitchOffTimer = 0;
  }

  if( !mHighlightingOn )
    return; // not highlighted

  if( mFlashTimer->isActive() )
    mFlashTimer->stop();

  QWidget * target = dynamic_cast< QWidget * >( parent() );
  if( !target )
  {
    deleteLater();
    return; // hum
  }

  target->setPalette( mSavedPalette );
  target->setAutoFillBackground( mSavedAutoFillBackground );

  mHighlightingOn = false;

  unhookEventFilter( target );

  deleteLater();
}

void WidgetHighlighter::slotSwitchHighlightingOff()
{
  switchHighlightingOff();
}

} // namespace Private

} // namespace UI

} // namespace Filter

} // namespace Akonadi

