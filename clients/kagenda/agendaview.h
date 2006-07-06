#ifndef AGENDAVIEW_H
#define AGENDAVIEW_H

#include <QWidget>

class StripeView;
class QwtWheel;
class AgendaModel;
class ContactModel;
class DataProvider;

class AgendaView : public QWidget
{
    Q_OBJECT
  public:
    AgendaView( QWidget *parent = 0 );

    void setDataProvider( DataProvider * );

  protected slots:
    void slotCoarseWheelValueChanged( double );
    void slotFineWheelValueChanged( double );

    void slotCellPressed( int );

    void closeView();
    void save();

  private:
    DataProvider *mDataProvider;

    StripeView *mStripe;

    QwtWheel *mFineWheel;
    QwtWheel *mCoarseWheel;
};

#endif
