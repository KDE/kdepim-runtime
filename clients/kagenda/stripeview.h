#ifndef STRIPEVIEW_H
#define STRIPEVIEW_H

#include <QWidget>
#include <QTimeLine>
#include <QMap>

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

  public slots:
    void setTopPosition( int position );
    void movePosition( int delta );
    void centerCell( int position );
    void zoomCell( int position );
    void unzoomCell( int position );

  signals:
    void valueChanged( int position );
    void cellPressed( int position );

  protected:
    void mousePressEvent ( QMouseEvent * event );
    void paintEvent( QPaintEvent * );

  protected slots:
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
