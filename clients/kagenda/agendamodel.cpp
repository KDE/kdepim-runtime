/*
    This file is part of Akonadi.

    Copyright (c) 2006 Cornelius Schumacher <schumacher@kde.org>

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

#include "agendamodel.h"

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

AgendaModel::AgendaModel()
{
}

QString AgendaModel::label( int position ) const
{
  QDate d = date( position );
  QString label = QDate::shortDayName( d.dayOfWeek() );
  label += ' ' + QString::number( d.day() );
  return label;
}

QColor AgendaModel::cellColor( int position ) const
{
  QDate d = date( position );
  if ( d.dayOfWeek() == 7 ) return QColor( "red" );
  if ( d.dayOfWeek() == 6 ) return QColor( 255, 128, 128 );

  return QColor();
}

QColor AgendaModel::decorationColor( int position ) const
{
  QDate d = date( position );

  int month = d.month();
  bool isOddMonth = month % 2 == 0;

  QColor monthColor;
  if ( isOddMonth ) {
    monthColor = "yellow";
  } else {
    monthColor = "orange";
  }

  return monthColor;
}

QString AgendaModel::decorationLabel( int position ) const
{
  QDate d = date( position );
  if ( d.day() != 1 ) return QString();

  return QDate::longMonthName( d.month() ) + ' ' + QString::number( d.year() );
}

bool AgendaModel::hasDecoration() const
{
  return true;
}

QDate AgendaModel::date( int position ) const
{
  QDate date( 2006, 1, 1 );
  return date.addDays( position );
}

void AgendaModel::addEvent( const Event &e )
{
  mEvents.append( e );
}

void AgendaModel::paintCell( int position, QPainter *p, const QRect &rect )
{
  QDate d = date( position );

  Event::List currentEvents;
  foreach( Event e, mEvents ) {
    if ( e.start.isValid() && e.start.date() == d &&
         e.end.isValid() && e.end.date() == d ) {
      currentEvents.append( e );
    }
  }

  foreach( Event e, currentEvents ) {
    QTime startTime = e.start.time();
    QTime endTime = e.end.time();

    int startX = xForTime( rect, startTime );
    int endX = xForTime( rect, endTime );

    p->fillRect( startX, rect.top(), endX - startX, rect.height(),
      QBrush( "green" ) );
  }

  int margin = 10;
  int maxcount = 5;
  int spacing = 8;

  if ( rect.height() > 2 * margin ) {
    int count = currentEvents.size();

    if ( count > 0 ) {
      if ( count > maxcount ) count = maxcount;

      int summaryHeight =
        ( rect.height() - 2 * margin - ( count - 1 ) * spacing ) / count;

      for( int i = 0; i < count; ++i ) {
        Event event = currentEvents[ i ];

        int top = i * ( summaryHeight + spacing ) + margin + rect.top();

        QRect summaryRect( rect.left() + margin, top,
          rect.width() - 2 * margin, summaryHeight );

        QColor markerColor( "grey" );
        markerColor.setAlpha( 220 );

        int left = xForTime( rect, event.start.time() );

        QPainterPath leftMarker;
        leftMarker.moveTo( left, top - 4 );
        leftMarker.lineTo( left, top );
        leftMarker.lineTo( left - 4, top );

        p->fillPath( leftMarker, markerColor );

        int right = xForTime( rect, event.end.time() );

        QPainterPath rightMarker;
        rightMarker.moveTo( right, top - 4 );
        rightMarker.lineTo( right, top );
        rightMarker.lineTo( right + 4, top );

        p->fillPath( rightMarker, markerColor );

        drawEventSummary( p, event, summaryRect );
      }
    }
  }
}

void AgendaModel::drawEventSummary( QPainter *p, const Event &event,
  const QRect &rect )
{
  QColor c( "grey" );
  c.setAlpha( 220 );
  p->fillRect( rect, c );

  QString text = event.title;

  if ( rect.height() > 16 ) {
    int textTop = rect.top() + 12;
    if ( rect.height() > 26 ) {
      textTop += 14;
      QString timeText = timeString( event.start.time() ) + '-' +
        timeString( event.end.time() );
      p->drawText( rect.left() + 4, rect.top() + 12, timeText );
    }
    p->drawText( rect.left() + 4, textTop, text );
  }
}

int AgendaModel::xForTime( const QRect &rect, const QTime &time ) const
{
  const int totalMinutes = 24 * 60;

  int minute = time.hour() * 60 + time.minute();

  int x = rect.left() + rect.width() * minute / totalMinutes;

  return x;
}

QString AgendaModel::timeString( const QTime &time )
{
  QString result = QString::number( time.hour() );
  result += ':';

  QString minuteString = QString::number( time.minute() );
  if ( minuteString.size() == 1 ) minuteString.prepend( "0" );

  result += minuteString;

  return result;
}

Event::List AgendaModel::events() const
{
  return mEvents;
}

void AgendaModel::clear()
{
  mEvents.clear();
}
