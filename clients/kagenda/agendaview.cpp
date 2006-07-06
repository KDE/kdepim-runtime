#include "agendaview.h"

#include "stripeview.h"
#include "qwt_wheel.h"
#include "agendamodel.h"
#include "dummymodel.h"
#include "contactmodel.h"
#include "dataprovider.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QDebug>
#include <QTime>
#include <QPushButton>

AgendaView::AgendaView( QWidget *parent )
  : QWidget( parent )
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this );

  QBoxLayout *topLayout = new QHBoxLayout();
  mainLayout->addLayout( topLayout );
  
  mStripe = new StripeView( this );
  topLayout->addWidget( mStripe, 1 );
  connect( mStripe, SIGNAL( cellPressed( int ) ), SLOT( slotCellPressed( int ) ) );

  mFineWheel = new QwtWheel( this );
  topLayout->addWidget( mFineWheel );
  mFineWheel->setOrientation( Qt::Vertical );
  mFineWheel->setRange( -100, 100 );
  mFineWheel->setMass( 0.5 );
  mFineWheel->setStep( 1 );
  mFineWheel->setPeriodic( true );
  mFineWheel->setTotalAngle( 20 );
  connect( mFineWheel, SIGNAL( valueDelta( double ) ),
    SLOT( slotFineWheelValueChanged( double ) ) );

  mCoarseWheel = new QwtWheel( this );
  topLayout->addWidget( mCoarseWheel );
  mCoarseWheel->setOrientation( Qt::Vertical );
  mCoarseWheel->setRange( -1000, 1000 );
  mCoarseWheel->setMass( 0.5 );
  mCoarseWheel->setTotalAngle( 40 );
  mCoarseWheel->setPeriodic( true );
  connect( mCoarseWheel, SIGNAL( valueDelta( double ) ),
    SLOT( slotCoarseWheelValueChanged( double ) ) );

  QSlider *slider = new QSlider( Qt::Vertical, this );
  topLayout->addWidget( slider );
  slider->setMinimum( -1000 );
  slider->setMaximum( 1000 );
  connect( slider, SIGNAL( sliderMoved( int ) ),
    mStripe, SLOT( setTopPosition( int ) ) );

  connect( mStripe, SIGNAL( valueChanged( int ) ),
    slider, SLOT( setValue( int ) ) );

  QPushButton *button = new QPushButton( "&Save", this );
  mainLayout->addWidget( button );
  connect( button, SIGNAL( clicked() ), SLOT( save() ) );

  button = new QPushButton( "&Close", this );
  mainLayout->addWidget( button );
  connect( button, SIGNAL( clicked() ), SLOT( closeView() ) );

  resize( 300, 800 );
}

void AgendaView::slotCoarseWheelValueChanged( double value )
{
  mStripe->movePosition( int( value ) );
}

void AgendaView::slotFineWheelValueChanged( double value )
{
  mStripe->movePosition( int( value ) );
}

void AgendaView::slotCellPressed( int position )
{
  mStripe->centerCell( position );
  mStripe->unzoomCell( mStripe->zoomPosition() );
  mStripe->zoomCell( position );
}

void AgendaView::setDataProvider( DataProvider *provider )
{
  mDataProvider = provider;
  
  provider->load();
  mStripe->setModel( provider->model() );
}

void AgendaView::closeView()
{
  qDebug() << "closeView()";
  mDataProvider->save();
  qDebug() << "saved";
  close();
  qDebug() << "closed";
}

void AgendaView::save()
{
  mDataProvider->save();
}

#include "agendaview.moc"
