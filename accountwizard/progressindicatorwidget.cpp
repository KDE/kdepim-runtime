/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "progressindicatorwidget.h"

#include <QTimer>

IndicatorProgress::IndicatorProgress(ProgressIndicatorWidget *widget, QObject *parent)
    : QObject(parent),
      mProgressCount(0),
      mIndicator(widget)
{
    mProgressPix = KPixmapSequence(QLatin1String("process-working"), KIconLoader::SizeSmallMedium);
    mProgressTimer = new QTimer(this);
    connect(mProgressTimer, SIGNAL(timeout()), this, SLOT(slotTimerDone()));
}

IndicatorProgress::~IndicatorProgress()
{
}

void IndicatorProgress::slotTimerDone()
{
    mIndicator->setPixmap(mProgressPix.frameAt(mProgressCount));
    ++mProgressCount;
    if (mProgressCount == 8)
        mProgressCount = 0;

    mProgressTimer->start(300);
}

void IndicatorProgress::startAnimation()
{
    mProgressCount = 0;
    mProgressTimer->start(300);
}

void IndicatorProgress::stopAnimation()
{
    if (mProgressTimer->isActive())
        mProgressTimer->stop();
    mIndicator->clear();
}

class ProgressIndicatorWidgetPrivate
{
public:
    ProgressIndicatorWidgetPrivate(ProgressIndicatorWidget *qq)
        : q(qq)
    {
        indicator = new IndicatorProgress(q);
    }

    ~ProgressIndicatorWidgetPrivate()
    {
        delete indicator;
    }

    IndicatorProgress *indicator;
    ProgressIndicatorWidget *q;
};

ProgressIndicatorWidget::ProgressIndicatorWidget(QWidget *parent)
    : QLabel(parent),
      d(new ProgressIndicatorWidgetPrivate(this))
{
    setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed ) );
}

ProgressIndicatorWidget::~ProgressIndicatorWidget()
{
    delete d;
}

void ProgressIndicatorWidget::start()
{
    d->indicator->startAnimation();
}

void ProgressIndicatorWidget::stop()
{
    d->indicator->stopAnimation();
}

#include "progressindicatorwidget.moc"
