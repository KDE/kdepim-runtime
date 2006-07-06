#include "dummymodel.h"

#include <QPainter>

DummyModel::DummyModel()
{
}

QString DummyModel::label( int position ) const
{
  return QString::number( position );
}

QColor DummyModel::cellColor( int position ) const
{
  if ( position % 2 == 0 ) return QColor( 183, 216, 252 );
  else return QColor( "yellow" );
}

QColor DummyModel::decorationColor( int position ) const
{
  if ( position / 10 % 2 == 0 ) return QColor( "#EEC19F" );
  else return QColor( "#A1EEA6" );
}

QString DummyModel::decorationLabel( int position ) const
{
  if ( position % 10 != 0 ) return QString();

  return QString::number( int( position / 10 ) );
}

bool DummyModel::hasDecoration() const
{
  return true;
}
