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

#ifndef CANNEDDATAMODEL_H
#define CANNEDDATAMODEL_H

#include <QStandardItemModel>

class FakeCollection : public QObject
{
  Q_OBJECT

public:
  explicit FakeCollection(const QString &colName, QObject* parent = 0);

  QString colName() const;

private:
  QString m_colName;

};

class FakeContact : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(QString email READ email NOTIFY emailChanged)
  Q_PROPERTY(QString address READ address NOTIFY addressChanged)
  Q_PROPERTY(QPixmap image READ image NOTIFY imageChanged)
  Q_PROPERTY(QString phone READ phone NOTIFY phoneChanged)
public:
  FakeContact( const QVariantList data, QObject *parent = 0 );

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
  QVariantList m_data;
};


class FakeEntityTreeModel : public QStandardItemModel
{
  Q_OBJECT
public:
  enum DataType {
    ContactsData,
    CalendarData
  };

  FakeEntityTreeModel( DataType dataType, QObject* parent = 0);

private:
  void initContactData();
  void initCalendarData();

};

class FakeEntityTreeItem : public QStandardItem
{
public:
  FakeEntityTreeItem( QObject *wrappedObject );

  virtual QVariant data(int role = Qt::UserRole + 1) const;

private:
  QObject *m_containedObject;

};

#endif
