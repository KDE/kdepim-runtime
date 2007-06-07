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

#include "stripeview.h"

#include <QtCore/QDebug>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

#include <assert.h>

StripeView::Model::Model()
{
}

StripeView::Model::~Model()
{
}

QString StripeView::Model::label( int position ) const
{
  return QString::number( position );
}

QColor StripeView::Model::cellColor( int ) const
{
  return QColor();
}

QColor StripeView::Model::decorationColor( int ) const
{
  return QColor();
}

bool StripeView::Model::hasDecoration() const
{
  return false;
}

QString StripeView::Model::decorationLabel( int ) const
{
  return QString();
}

void StripeView::Model::paintCell( int, QPainter *,
  const QRect & )
{
}


StripeView::StripeView( QWidget *parent )
  : QWidget( parent ), mDefaultCellHeight( 20 ),
    mZoomedCellHeight( mDefaultCellHeight * 4 ), mTopPosition( 0 ),
    mPixelOffset( 0 ),
    mDecorationWidth( 20 ),
    mModel( 0 ), mZoomPosition( 0 ), mUnzoomPosition( 0 )
{
  int period = 700;

  mCenterTimeLine.setDuration( period );
  connect( &mCenterTimeLine, SIGNAL( frameChanged( int ) ),
    SLOT( slotCenterFrameChanged( int ) ) );

  mZoomTimeLine.setDuration( period );
  connect( &mZoomTimeLine, SIGNAL( frameChanged( int ) ),
    SLOT( slotZoomFrameChanged( int ) ) );

  mUnzoomTimeLine.setDuration( period );
  connect( &mUnzoomTimeLine, SIGNAL( frameChanged( int ) ),
    SLOT( slotUnzoomFrameChanged( int ) ) );

#if 0
  setCellHeight( 0, 40 );
  setCellHeight( 10, 40 );
  setCellHeight( 15, 50 );
#endif
}

void StripeView::paintEvent( QPaintEvent * )
{
  assert( mModel );

  QPainter painter( this );

  painter.setRenderHint(QPainter::Antialiasing);

  QPen linePen;

  QRect r = contentsRect();
  
  int position = mTopPosition;

  int decorationWidth;
  if ( mModel->hasDecoration() ) {
    decorationWidth = mDecorationWidth;
  } else {
    decorationWidth = 0;
  }
  
  int cellLeft = r.left() + decorationWidth;
  int cellRight = r.right();
  int cellWidth = r.right() - cellLeft;
  int cellTop = r.top() - mPixelOffset;
  
  while( cellTop <= r.bottom() ) {
    int cellHeight = StripeView::cellHeight( position );
  
    if ( mModel->hasDecoration() ) {
      QColor decorationColor = mModel->decorationColor( position );
      if ( decorationColor.isValid() ) {
        painter.fillRect( r.left(), cellTop, decorationWidth, cellHeight,
          QBrush( decorationColor ) );
      }
    }

    QColor cellColor = mModel->cellColor( position );  
    if ( cellColor.isValid() ) {
      QPen pen;
      pen.setColor( cellColor );
      painter.setPen( pen );
      painter.fillRect( cellLeft, cellTop, cellWidth, cellHeight,
        QBrush( cellColor ) );
    }

    mModel->paintCell( position, &painter, QRect( cellLeft, cellTop, cellWidth,
      cellHeight ) );
    
    painter.setPen( linePen );
    painter.drawLine( cellLeft, cellTop, cellRight, cellTop );
    painter.drawText( cellLeft, cellTop + cellHeight - 5,
      mModel->label( position ) );

    cellTop += cellHeight;
    position++;
  }

  if ( mModel->hasDecoration() ) {
    position = mTopPosition;
    cellTop = r.top() - mPixelOffset;

    while( cellTop <= r.bottom() ) {
      QString decorationLabel = mModel->decorationLabel( position );
      if ( !decorationLabel.isEmpty() ) {
        QPainterPath path;
        path.addText( 0, 0, font(), decorationLabel );
        QMatrix matrix;
        matrix.rotate( 270 );
        path = matrix.map( path );

        QRectF textRect = path.boundingRect();
        matrix = QMatrix();
        matrix.translate( textRect.width() + 4, cellTop + textRect.height() + 8 );
        path = matrix.map( path );      

        painter.fillPath( path, QBrush( "black" ) );
      }

      cellTop += StripeView::cellHeight( position );
      position++;
    }
  }
}

QSize StripeView::minimumSizeHint () const
{
  return QSize( 100, 100 );
}

int StripeView::topPosition() const
{
  return mTopPosition;
}

int StripeView::positionForPos( int yPos ) const
{
  int position = mTopPosition;
  int y = -mPixelOffset;
  
  while ( y < yPos ) {
    y += cellHeight( position++ );
  }
  
  return position - 1;
}

int StripeView::bottomPosition() const
{
  return positionForPos( contentsRect().bottom() );
}

void StripeView::setTopPosition( int value )
{
  mTopPosition = value;
  emit valueChanged( mTopPosition );
  update();
}

void StripeView::movePosition( int delta )
{
  if ( delta == 0 ) return;

#if 0
  qDebug() << "DELTA: " << delta;
#endif

#if 0
  qDebug() << "  ORIG PIXELOFFSET:" << mPixelOffset;
  qDebug() << "  ORIG POSITION:" << mTopPosition;
#endif

  mPixelOffset -= delta;

  int position = mTopPosition;
  if ( mPixelOffset < 0 ) {
    while( mPixelOffset < 0 ) {
      position--;
      mPixelOffset += cellHeight( position );
    }
  } else {
    while( mPixelOffset > cellHeight( position ) ) {
      mPixelOffset -= cellHeight( position );
      position++;
    }
  }

#if 0
  qDebug() << "  FINAL PIXELOFFSET:" << mPixelOffset;
#endif

  if ( position != mTopPosition ) {
    mTopPosition = position;
    emit valueChanged( mTopPosition );
  }
#if 0
  qDebug() << "  NEW BASE POSITION" << mTopPosition;
#endif

  update();
}

void StripeView::setModel( Model *model )
{
  mModel = model;
}

StripeView::Model *StripeView::model() const
{
  return mModel;
}

void StripeView::mousePressEvent ( QMouseEvent * event )
{
//  qDebug() << "CLICKED POS: " << event->pos();

  QWidget::mousePressEvent( event );

  if ( mModel->hasDecoration() ) {
    if ( event->x() <= mDecorationWidth ) {
      qDebug() << "DECORATION CLICKED";
    } else {
      int position = positionForPos( event->y() );
      qDebug() << "CLICKED: " << mModel->label( position );
      emit cellPressed( position );
    }
  }
}

void StripeView::centerCell( int position )
{
  int centerOffset = contentsRect().height() / 2;

  centerOffset -= cellHeight( position ) / 2;  
  
  int topPosition = position;
  while ( centerOffset > 0 ) {
    centerOffset -= cellHeight( --topPosition );
  }
  
  mCenterDelta = 0;
  
  int p = mTopPosition;
  if ( topPosition > mTopPosition ) {
    while ( p <= topPosition ) {
      mCenterDelta += cellHeight( p );
      p++;
    }
  } else {
    while ( p >= topPosition ) {
      mCenterDelta -= cellHeight( p );
      p--;
    }
  }

//  qDebug() << "CENTER DELTA:" << mCenterDelta;

  mCenterTimeLine.stop();    
  mCenterTimeLine.setFrameRange( mCenterDelta, 0 );
  mCenterTimeLine.start();
    
//  setTopPosition( position - maxCells / 2 );
}

void StripeView::slotCenterFrameChanged( int value )
{
//  qDebug() << "VALUE" << value << "CENTERDELTA" << mCenterDelta;

  int delta = value - mCenterDelta;
  mCenterDelta = value;

//  qDebug() << "DELTA" << delta;

  movePosition( delta );
}

void StripeView::zoomCell( int position )
{
  mZoomPosition = position;
  
  mZoomTimeLine.setFrameRange( cellHeight( position ), mZoomedCellHeight );
  mZoomTimeLine.start();
}

void StripeView::slotZoomFrameChanged( int value )
{
  setCellHeight( mZoomPosition, value );
  
  update();
}

void StripeView::unzoomCell( int position )
{
  mUnzoomPosition = position;
  
  mUnzoomTimeLine.setFrameRange( cellHeight( position ), mDefaultCellHeight );
  mUnzoomTimeLine.start();
}

void StripeView::slotUnzoomFrameChanged( int value )
{
  setCellHeight( mUnzoomPosition, value );
  
  update();
}

int StripeView::cellHeight( int position ) const
{
  QMap<int,int>::ConstIterator it = mCellHeights.find( position );
  if ( it == mCellHeights.end() ) return mDefaultCellHeight;
  else return it.value();
}

void StripeView::setCellHeight( int position, int cellHeight )
{
  mCellHeights.insert( position, cellHeight );
}

int StripeView::zoomPosition() const
{
  return mZoomPosition;
}

#include "stripeview.moc"
