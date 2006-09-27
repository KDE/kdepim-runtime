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
#ifndef STRIPEVIEW_H
#define STRIPEVIEW_H

#include <QtCore/QMap>
#include <QtCore/QTimeLine>
#include <QtGui/QWidget>

class StripeView : public QWidget
{
    Q_OBJECT
  public:
    class Model {
      public:
        Model();
        virtual ~Model();

        virtual QString label( int position ) const;
        virtual QColor cellColor( int position ) const;
        virtual bool hasDecoration() const;
        virtual QColor decorationColor( int position ) const;
        virtual QString decorationLabel( int position ) const;
        virtual void paintCell( int position, QPainter *p, const QRect &rect );
    };

    StripeView( QWidget *parent = 0 );

    QSize minimumSizeHint () const;

    void setModel( Model * );
    Model *model() const;

    int topPosition() const;
    int bottomPosition() const;

    int cellHeight( int position ) const;
    void setCellHeight( int position, int cellHeight );

    int positionForPos( int y ) const;

    int zoomPosition() const;

  public Q_SLOTS:
    void setTopPosition( int position );
    void movePosition( int delta );
    void centerCell( int position );
    void zoomCell( int position );
    void unzoomCell( int position );

  Q_SIGNALS:
    void valueChanged( int position );
    void cellPressed( int position );

  protected:
    void mousePressEvent ( QMouseEvent * event );
    void paintEvent( QPaintEvent * );

  protected Q_SLOTS:
    void slotCenterFrameChanged( int value );
    void slotZoomFrameChanged( int value );
    void slotUnzoomFrameChanged( int value );

  private:
    int mDefaultCellHeight;
    int mZoomedCellHeight;
    QMap<int, int> mCellHeights;

    int mTopPosition;
    int mPixelOffset;

    int mDecorationWidth;

    Model *mModel;
    
    QTimeLine mCenterTimeLine;
    int mCenterDelta;

    QTimeLine mZoomTimeLine;
    int mZoomPosition;

    QTimeLine mUnzoomTimeLine;
    int mUnzoomPosition;
};

#endif
