#include "contactmodel.h"

#include <QPainter>
#include <QPainterPath>

ContactModel::ContactModel()
{
}

QString ContactModel::label( int position ) const
{
  if ( position < 0 || position >= mContacts.size() ) return QString();
  
  return mContacts[ position ].name;
}

QColor ContactModel::cellColor( int position ) const
{
  if ( position < 0 || position >= mContacts.size() ) {
    return QColor( "red" );
  }

  return QColor();
}

QColor ContactModel::decorationColor( int position ) const
{
  if ( position < 0 || position >= mContacts.size() ) {
    return QColor( "red" );
  }

  QChar firstChar = mContacts[ position ].name.at( 0 );
  if ( firstChar.unicode() % 2 == 0 ) return QString( "#51A5EE" );
  else return QString( "#96C9EE" );
}

QString ContactModel::decorationLabel( int position ) const
{
  if ( position < 0 || position >= mContacts.size() ) {
    return QString();
  }

  QChar firstChar = mContacts[ position ].name.at( 0 );

  if ( position > 0 ) {
    QChar previousFirstChar = mContacts[ position - 1 ].name.at( 0 );
    if ( previousFirstChar == firstChar ) return QString();
  }
  
  return firstChar;
}

bool ContactModel::hasDecoration() const
{
  return true;
}

void ContactModel::addContact( const Contact &e )
{
  mContacts.append( e );
}

void ContactModel::paintCell( int position, QPainter *p, const QRect &rect )
{
  if ( position < 0 || position >= mContacts.size() ) {
    return;
  }

  Contact contact = mContacts[ position ];

  int margin = 10;

  if ( rect.height() > 2 * margin ) {
    QColor c( "grey" );
    c.setAlpha( 220 );

    QRect box( rect.left() + margin, rect.top() + margin,
      rect.width() - 2 * margin, rect.height() - 3 * margin );

    p->fillRect( box, c );

    if ( box.height() > 16 ) {
      p->drawText( box.left() + 4, box.top() + 12, contact.email );
      if ( box.height() > 32 ) {
        p->drawText( box.left() + 4, box.top() + 28, contact.phone );
      }
    }
  }
}
