/* This file is part of the KDE libraries
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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef __KPIXMAPREGIONSELECTORWIDGET_H__
#define __KPIXMAPREGIONSELECTORWIDGET_H__

#include <qvbox.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qlabel.h>
#include <kimageeffect.h>

class KPopupMenu;

namespace KPIM {

/**
 * KPixmapRegionSelectorWidget is a widget that shows a picture and provides the
 * user with a friendly way to select a rectangular subregion of the pixmap.
 *
 * @author Antonio Larrosa <larrosa@kde.org>
 * @since 3.4
 */
class KPixmapRegionSelectorWidget : public QWidget
{
   Q_OBJECT

public:
   /**
    * Constructor for a KPixmapRegionSelectorWidget.
    */
   KPixmapRegionSelectorWidget( QWidget *parent = 0L, const char *name=0L);

   /**
    * Destructor for a KPixmapRegionSelectorWidget
    */
   ~KPixmapRegionSelectorWidget();

   /**
    * Sets the pixmap which will be shown for the user to select a region from.
    */
   void setPixmap( const QPixmap &pixmap );

   /**
    * @return the original whole pixmap that we're using in this widget as the
    * pixmap the user is selecting a region from.
    */
   QPixmap pixmap() const { return m_unzoomedPixmap; };

   /**
    * Sets the selected region to be @p rect (in zoomed pixmap coordinates)
    */
   void setSelectedRegion(const QRect &rect);

   /**
    * Returns the selected region ( in zoomed pixmap coordinates )
    */
   QRect selectedRegion() const;

   /**
    * Returns the selected region ( in unzoomed, original pixmap coordinates )
    */
   QRect unzoomedSelectedRegion() const;

   /**
    * Resets the selection to use the whole image
    */
   void resetSelection();

   /**
    * @returns a QImage object with just the region the user selected from the
    * image
    */
   QImage selectedImage() const;

   /**
    * Sets the aspect ration that the selected subimage should have. The way to
    * select it, is specifying an example valid @p width and @p height.
    * @see setFreeSelectionAspectRatio()
    */
   void setSelectionAspectRatio(int width, int height);

   /**
    * Allows the user to do a selection which has any aspect ratio. This is
    * the default.
    * @see setSelectionAspectRatio()
    */
   void setFreeSelectionAspectRatio();
 
   /**
    * Sets the maximum size for the widget. If the image is larger than this
    * (either horizontally or vertically), it's scaled to adjust to the maximum
    * size (preserving the aspect ratio)
    */
   void setMaximumWidgetSize( int width, int height );

   /**
    * Rotates the image as specified by the @p direction parameter, also tries
    * to rotate the selected region so that it doesn't change, as long as the
    * forced aspect ratio setting is respected, in other case, the selected region
    * is resetted.
    */
   void rotate(KImageEffect::RotateDirection direction);

public slots:
   /**
    * Rotates the current image 90º clockwise 
    */
   void rotateClockwise();
   /**
    * Rotates the current image 90º counterclockwise
    */
   void rotateCounterclockwise();

protected:
   /**
    * Creates a KPopupMenu with the menu that appears when clicking with the right button on the label
    */
   virtual KPopupMenu *createPopupMenu();


private:
   bool eventFilter(QObject *obj, QEvent *ev);

   /**
    * Recalculates the pixmap that is shown based on the current selected area,
    * the original image, etc. 
    */
   void updatePixmap();

   QRect calcSelectionRectangle( const QPoint &startPoint, const QPoint & endPoint );

   enum CursorState { None=0, Resizing, Moving };
   CursorState m_state;

   QPixmap m_unzoomedPixmap;
   QPixmap m_originalPixmap;
   QPixmap m_linedPixmap;
   QRect   m_selectedRegion;
   QLabel *m_label;

   QPoint m_tempFirstClick;
   double m_forcedAspectRatio;

   int m_maxWidth, m_maxHeight;
   double m_zoomFactor;
};

}; // for namespace

#endif
