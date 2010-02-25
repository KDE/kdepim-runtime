/*
* This file is part of Akonadi
*
* Copyright 2010 Stephen Kelly <steveire@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#ifndef EVENT_WRAPPER_H
#define EVENT_WRAPPER_H

#include <QDateTime>
#include <KCal/Incidence>

#include <boost/shared_ptr.hpp>

class QPixmap;

class EventWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
  Q_PROPERTY(QString location READ location NOTIFY locationChanged)
  Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
  Q_PROPERTY(QPixmap image READ image NOTIFY imageChanged)

public:
  EventWrapper(KCal::Incidence::Ptr incidence, QObject* parent = 0);

  KCal::Incidence::Ptr incidence() const;
  QString summary() const;
  QString location() const;
  QString description() const;
  QPixmap image() const;

signals:
  void summaryChanged();
  void locationChanged();
  void imageChanged();
  void descriptionChanged();

private:
  KCal::Incidence::Ptr m_incidence;
};

#endif
