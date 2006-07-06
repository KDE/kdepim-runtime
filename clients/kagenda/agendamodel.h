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
