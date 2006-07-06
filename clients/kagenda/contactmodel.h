#ifndef CONTACTMODEL_H
#define CONTACTMODEL_H

#include "stripeview.h"
#include "contact.h"

#include <QDate>

class ContactModel : public StripeView::Model
{
  public:
    ContactModel();

    QString label( int position ) const;
    QColor cellColor( int position ) const;
    bool hasDecoration() const;
    QColor decorationColor( int position ) const;
    QString decorationLabel( int position ) const;

    void paintCell( int position, QPainter *p, const QRect &rect );

    void addContact( const Contact & );

  protected:

  private:
    Contact::List mContacts;
};

#endif
