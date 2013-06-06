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

#ifndef PROGRESSINDICATORWIDGET_H
#define PROGRESSINDICATORWIDGET_H

#include <KPixmapSequence>

#include <QLabel>

class QTimer;
class ProgressIndicatorWidget;
class IndicatorProgress : public QObject
{
    Q_OBJECT
public:
    explicit IndicatorProgress(ProgressIndicatorWidget *widget, QObject *parent = 0);
    ~IndicatorProgress();

    void startAnimation();
    void stopAnimation();

private Q_SLOTS:
    void slotTimerDone();

private:
    int mProgressCount;
    KPixmapSequence mProgressPix;
    QTimer *mProgressTimer;
    ProgressIndicatorWidget *mIndicator;
};

class ProgressIndicatorWidgetPrivate;
class ProgressIndicatorWidget : public QLabel
{
    Q_OBJECT
public:
    explicit ProgressIndicatorWidget(QWidget *parent = 0);
    ~ProgressIndicatorWidget();

public Q_SLOTS:
    void start();
    void stop();

private:
    friend class ProgressIndicatorWidgetPrivate;
    ProgressIndicatorWidgetPrivate * const d;
};

#endif // PROGRESSINDICATORWIDGET_H
