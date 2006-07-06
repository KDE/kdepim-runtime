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
#ifndef AGENDAMODEL_H
#define AGENDAMODEL_H

#include "stripeview.h"
#include "event.h"

#include <QDate>

class AgendaModel : public StripeView::Model
{
  public:
    AgendaModel();

    QString label( int position ) const;
    QColor cellColor( int position ) const;
    bool hasDecoration() const;
    QColor decorationColor( int position ) const;
    QString decorationLabel( int position ) const;

    void paintCell( int position, QPainter *p, const QRect &rect );

    void clear();

    void addEvent( const Event & );

    Event::List events() const;

  protected:
    QDate date( int position ) const;

    void drawEventSummary( QPainter *p, const Event &event, const QRect &rect );

    int xForTime( const QRect &rect, const QTime &time ) const;

    QString timeString( const QTime &time );

  private:
    Event::List mEvents;
};

#endif
