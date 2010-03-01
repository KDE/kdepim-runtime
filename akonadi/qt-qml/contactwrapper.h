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

#include <QDebug>

#include <KABC/Addressee>

class DeclarativeContact : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(QString email READ email NOTIFY emailChanged)
  Q_PROPERTY(QString address READ address NOTIFY addressChanged)
  Q_PROPERTY(QPixmap image READ image NOTIFY imageChanged)
  Q_PROPERTY(QString phone READ phone NOTIFY phoneChanged)
public:
  DeclarativeContact( KABC::Addressee addressee, QObject *parent = 0 );

  QString name() const;
  QString email() const;
  QString address() const;
  QPixmap image() const;
  QString phone() const;

signals:
  void nameChanged();
  void emailChanged();
  void addressChanged();
  void imageChanged();
  void phoneChanged();

private:
  KABC::Addressee m_addressee;
};
