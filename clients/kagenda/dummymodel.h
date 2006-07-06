#ifndef DUMMYMODEL_H
#define DUMMYMODEL_H

#include "stripeview.h"
#include "event.h"

#include <QDate>

class DummyModel : public StripeView::Model
{
  public:
    DummyModel();

    QString label( int position ) const;
    QColor cellColor( int position ) const;
    bool hasDecoration() const;
    QColor decorationColor( int position ) const;
    QString decorationLabel( int position ) const;
};

#endif
