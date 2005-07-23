/*
    This file is part of libkdepim.

    Copyright (C) 2004 Antonio Larrosa <larrosa@kde.org

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/* NOTE: There are two copies of this .h and the .cpp file, with subtle differences.
 * One copy is in kdelibs/kdeui, and the other copy is in kdepim/libkdepim
 * This is because kdepim has to remain backwards compatible.  Any changes
 * to either file should be made to the other.
 */

#include "kpixmapregionselectorwidget.h"
#include <qpainter.h>
#include <qcolor.h>
#include <qimage.h>
#include <qlayout.h>
#include <kimageeffect.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <stdlib.h>
#include <qcursor.h>
#include <qapplication.h>

using namespace KPIM;

KPixmapRegionSelectorWidget::KPixmapRegionSelectorWidget( QWidget *parent,
      const char *name) : QWidget( parent, name)
{
   QHBoxLayout * hboxLayout=new QHBoxLayout( this );

   hboxLayout->addStretch();
   QVBoxLayout * vboxLayout=new QVBoxLayout( hboxLayout );

   vboxLayout->addStretch();
   m_label = new QLabel(this, "pixmapHolder");
   m_label->setBackgroundMode( Qt::NoBackground );
   m_label->installEventFilter( this );

   vboxLayout->addWidget(m_label);
   vboxLayout->addStretch();

   hboxLayout->addStretch();

   m_forcedAspectRatio=0;

   m_zoomFactor=1.0;
}

KPixmapRegionSelectorWidget::~KPixmapRegionSelectorWidget()
{
}

void KPixmapRegionSelectorWidget::setPixmap( const QPixmap &pixmap )
{
   Q_ASSERT(!pixmap.isNull()); //This class isn't designed to deal with null pixmaps.
   m_originalPixmap = pixmap;
   m_unzoomedPixmap = pixmap;
   m_label->setPixmap( pixmap );
   resetSelection();
}

void KPixmapRegionSelectorWidget::resetSelection()
{
   m_selectedRegion = m_originalPixmap.rect();
   updatePixmap();
}

QRect KPixmapRegionSelectorWidget::selectedRegion() const
{
     return m_selectedRegion;
}

void KPixmapRegionSelectorWidget::setSelectedRegion(const QRect &rect)
{
   if (!rect.isValid()) resetSelection();
   else
   {
      m_selectedRegion=rect;
      updatePixmap();

      QRect r=unzoomedSelectedRegion();
   }
}

void KPixmapRegionSelectorWidget::updatePixmap()
{
   Q_ASSERT(!m_originalPixmap.isNull()); if(m_originalPixmap.isNull()) { m_label->setPixmap(m_originalPixmap); return; }
   if (m_selectedRegion.width()>m_originalPixmap.width()) m_selectedRegion.setWidth( m_originalPixmap.width() );
   if (m_selectedRegion.height()>m_originalPixmap.height()) m_selectedRegion.setHeight( m_originalPixmap.height() );

   QPainter painter;
   if (m_linedPixmap.isNull())
   {
     m_linedPixmap = m_originalPixmap;

     painter.begin(&m_linedPixmap);
     painter.setRasterOp( Qt::XorROP );
     painter.fillRect(0,0,m_linedPixmap.width(), m_linedPixmap.height(),
                  QBrush( QColor(255,255,255), Qt::BDiagPattern) );
     painter.end();

     QImage image=m_linedPixmap.convertToImage();
     image=KImageEffect::fade(image, 0.4, QColor(0,0,0));
     m_linedPixmap.convertFromImage(image);
   }

   QPixmap pixmap = m_linedPixmap;

   painter.begin(&pixmap);
   painter.drawPixmap( m_selectedRegion.topLeft(),
        m_originalPixmap, m_selectedRegion );

   painter.setPen( QColor(255,255,255) );
   painter.setRasterOp( Qt::XorROP );

   painter.drawRect( m_selectedRegion );

   painter.end();

   m_label->setPixmap(pixmap);
}

KPopupMenu *KPixmapRegionSelectorWidget::createPopupMenu()
{
   KPopupMenu *popup=new KPopupMenu(this, "PixmapRegionSelectorPopup");
   popup->insertTitle(i18n("Image Operations"));

   KAction *action = new KAction(i18n("&Rotate Clockwise"), "rotate_cw",
                                0, this, SLOT(rotateClockwise()),
                                popup, "rotateclockwise");
   action->plug(popup);

   action = new KAction(i18n("Rotate &Counterclockwise"), "rotate_ccw",
                                0, this, SLOT(rotateCounterclockwise()),
                                popup, "rotatecounterclockwise");
   action->plug(popup);

/*
   I wonder if it would be appropiate to have here an "Open with..." option to
   edit the image (antlarr)
*/
   return popup;
}

void KPixmapRegionSelectorWidget::rotate(KImageEffect::RotateDirection direction)
{
   int w=m_originalPixmap.width();
   int h=m_originalPixmap.height();
   QImage img=m_unzoomedPixmap.convertToImage();
   img= KImageEffect::rotate(img, direction);
   m_unzoomedPixmap.convertFromImage(img);

   img=m_originalPixmap.convertToImage();
   img= KImageEffect::rotate(img, direction);
   m_originalPixmap.convertFromImage(img);

   m_linedPixmap=QPixmap();

   if (m_forcedAspectRatio>0 && m_forcedAspectRatio!=1)
      resetSelection();
   else
   {
      switch (direction)
      {
         case ( KImageEffect::Rotate90 ):
            {
              int x=h-m_selectedRegion.y()-m_selectedRegion.height();
              int y=m_selectedRegion.x();
              m_selectedRegion.setRect(x, y, m_selectedRegion.height(), m_selectedRegion.width() );
              updatePixmap();
            } break;
         case ( KImageEffect::Rotate270 ):
            {
              int x=m_selectedRegion.y();
              int y=w-m_selectedRegion.x()-m_selectedRegion.width();
              m_selectedRegion.setRect(x, y, m_selectedRegion.height(), m_selectedRegion.width() );
              updatePixmap();
            } break;
         default: resetSelection();
      }
   }
}

void KPixmapRegionSelectorWidget::rotateClockwise()
{
   rotate(KImageEffect::Rotate90);
}

void KPixmapRegionSelectorWidget::rotateCounterclockwise()
{
   rotate(KImageEffect::Rotate270);
}



bool KPixmapRegionSelectorWidget::eventFilter(QObject *obj, QEvent *ev)
{
   if ( ev->type() == QEvent::MouseButtonPress )
   {
      QMouseEvent *mev= (QMouseEvent *)(ev);
      //kdDebug() << QString("click at  %1,%2").arg( mev->x() ).arg( mev->y() ) << endl;

      if ( mev->button() == RightButton )
      {
         KPopupMenu *popup = createPopupMenu( );
         popup->exec( mev->globalPos() );
         delete popup;
         return TRUE;
      };

      QCursor cursor;
      if ( m_selectedRegion.contains( mev->pos() )
          && m_selectedRegion!=m_originalPixmap.rect() )
      {
         m_state=Moving;
         cursor=QCursor(Qt::SizeAllCursor);
      }
      else
      {
         m_state=Resizing;
         cursor=QCursor(Qt::CrossCursor);
      }
      QApplication::setOverrideCursor(cursor);

      m_tempFirstClick=mev->pos();

      return TRUE;
   }

   if ( ev->type() == QEvent::MouseMove )
   {
      QMouseEvent *mev= (QMouseEvent *)(ev);

      //kdDebug() << QString("move to  %1,%2").arg( mev->x() ).arg( mev->y() ) << endl;

      if ( m_state == Resizing )
      {
         setSelectedRegion (
              calcSelectionRectangle( m_tempFirstClick, mev->pos() ) );
      }
      else if (m_state == Moving )
      {
         int mevx = mev->x();
         int mevy = mev->y();
         bool mouseOutside=false;
         if ( mevx < 0 )
         {
           m_selectedRegion.moveBy(-m_selectedRegion.x(),0);
           mouseOutside=true;
         }
         else if ( mevx > m_originalPixmap.width() )
         {
           m_selectedRegion.moveBy(m_originalPixmap.width()-m_selectedRegion.width()-m_selectedRegion.x(),0);
           mouseOutside=true;
         }
         if ( mevy < 0 )
         {
           m_selectedRegion.moveBy(0,-m_selectedRegion.y());
           mouseOutside=true;
         }
         else if ( mevy > m_originalPixmap.height() )
         {
           m_selectedRegion.moveBy(0,m_originalPixmap.height()-m_selectedRegion.height()-m_selectedRegion.y());
           mouseOutside=true;
         }
         if (mouseOutside) { updatePixmap(); return TRUE; };

         m_selectedRegion.moveBy( mev->x()-m_tempFirstClick.x(),
                                  mev->y()-m_tempFirstClick.y() );

         // Check that the region has not fallen outside the image
         if (m_selectedRegion.x() < 0)
            m_selectedRegion.moveBy(-m_selectedRegion.x(),0);
         else if (m_selectedRegion.right() > m_originalPixmap.width())
            m_selectedRegion.moveBy(-(m_selectedRegion.right()-m_originalPixmap.width()),0);

         if (m_selectedRegion.y() < 0)
            m_selectedRegion.moveBy(0,-m_selectedRegion.y());
         else if (m_selectedRegion.bottom() > m_originalPixmap.height())
            m_selectedRegion.moveBy(0,-(m_selectedRegion.bottom()-m_originalPixmap.height()));

         m_tempFirstClick=mev->pos();
         updatePixmap();
      }
      return TRUE;
   }

   if ( ev->type() == QEvent::MouseButtonRelease )
   {
      QMouseEvent *mev= (QMouseEvent *)(ev);

      if ( m_state == Resizing && mev->pos() == m_tempFirstClick)
         resetSelection();

      m_state=None;
      QApplication::restoreOverrideCursor();

      return TRUE;
   }

   QWidget::eventFilter(obj, ev);
   return FALSE;
}

QRect KPixmapRegionSelectorWidget::calcSelectionRectangle( const QPoint & startPoint, const QPoint & _endPoint )
{
   QPoint endPoint = _endPoint;
   if ( endPoint.x() < 0 ) endPoint.setX(0);
   else if ( endPoint.x() > m_originalPixmap.width() ) endPoint.setX(m_originalPixmap.width());
   if ( endPoint.y() < 0 ) endPoint.setY(0);
   else if ( endPoint.y() > m_originalPixmap.height() ) endPoint.setY(m_originalPixmap.height());
   int w=abs(startPoint.x()-endPoint.x());
   int h=abs(startPoint.y()-endPoint.y());

   if (m_forcedAspectRatio>0)
   {
      double aspectRatio=w/double(h);

      if (aspectRatio>m_forcedAspectRatio)
         h=(int)(w/m_forcedAspectRatio);
      else
         w=(int)(h*m_forcedAspectRatio);
   }

   int x,y;
   if ( startPoint.x() < endPoint.x() )
     x=startPoint.x();
   else
     x=startPoint.x()-w;
   if ( startPoint.y() < endPoint.y() )
     y=startPoint.y();
   else
     y=startPoint.y()-h;

   if (x<0)
   {
      w+=x;
      x=0;
      h=(int)(w/m_forcedAspectRatio);

      if ( startPoint.y() > endPoint.y() )
        y=startPoint.y()-h;
   }
   else if (x+w>m_originalPixmap.width())
   {
      w=m_originalPixmap.width()-x;
      h=(int)(w/m_forcedAspectRatio);

      if ( startPoint.y() > endPoint.y() )
        y=startPoint.y()-h;
   }
   if (y<0)
   {
      h+=y;
      y=0;
      w=(int)(h*m_forcedAspectRatio);

      if ( startPoint.x() > endPoint.x() )
        x=startPoint.x()-w;
   }
   else if (y+h>m_originalPixmap.height())
   {
      h=m_originalPixmap.height()-y;
      w=(int)(h*m_forcedAspectRatio);

      if ( startPoint.x() > endPoint.x() )
        x=startPoint.x()-w;
   }

   return QRect(x,y,w,h);
}

QRect KPixmapRegionSelectorWidget::unzoomedSelectedRegion() const
{
  return QRect((int)(m_selectedRegion.x()/m_zoomFactor),
               (int)(m_selectedRegion.y()/m_zoomFactor),
               (int)(m_selectedRegion.width()/m_zoomFactor),
               (int)(m_selectedRegion.height()/m_zoomFactor));
}

QImage KPixmapRegionSelectorWidget::selectedImage() const
{
   QImage origImage=m_unzoomedPixmap.convertToImage();
   return origImage.copy(unzoomedSelectedRegion());
}

void KPixmapRegionSelectorWidget::setSelectionAspectRatio(int width, int height)
{
   m_forcedAspectRatio=width/double(height);
}

void KPixmapRegionSelectorWidget::setFreeSelectionAspectRatio()
{
   m_forcedAspectRatio=0;
}

void KPixmapRegionSelectorWidget::setMaximumWidgetSize(int width, int height)
{
   m_maxWidth=width;
   m_maxHeight=height;

   m_originalPixmap=m_unzoomedPixmap;
   if (m_selectedRegion == m_originalPixmap.rect()) m_selectedRegion=QRect();

//   kdDebug() << QString(" original Pixmap :") << m_originalPixmap.rect() << endl;
//   kdDebug() << QString(" unzoomed Pixmap : %1 x %2 ").arg(m_unzoomedPixmap.width()).arg(m_unzoomedPixmap.height()) << endl;

   if ( !m_originalPixmap.isNull() &&
       ( m_originalPixmap.width() > m_maxWidth ||
         m_originalPixmap.height() > m_maxHeight ) )
   {
         /* We have to resize the pixmap to get it complete on the screen */
         QImage image=m_originalPixmap.convertToImage();
         m_originalPixmap.convertFromImage( image.smoothScale( width, height, QImage::ScaleMin ) );
         //m_originalPixmap.convertFromImage( KImageEffect::sample( image, width, height ) );
         double oldZoomFactor = m_zoomFactor;
         m_zoomFactor=m_originalPixmap.width()/(double)m_unzoomedPixmap.width();

         if (m_selectedRegion.isValid())
         {
            m_selectedRegion=
                  QRect((int)(m_selectedRegion.x()*m_zoomFactor/oldZoomFactor),
                        (int)(m_selectedRegion.y()*m_zoomFactor/oldZoomFactor),
                        (int)(m_selectedRegion.width()*m_zoomFactor/oldZoomFactor),
                        (int)(m_selectedRegion.height()*m_zoomFactor/oldZoomFactor) );
         }
   }

   if (!m_selectedRegion.isValid()) m_selectedRegion = m_originalPixmap.rect();

   m_linedPixmap=QPixmap();
   updatePixmap();
   resize(m_label->width(), m_label->height());
}

#include "kpixmapregionselectorwidget.moc"
