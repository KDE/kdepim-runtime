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


#include "fakeentitytreemodel.h"

#include <QImageReader>

#include "contactwrapper.h"
#include "eventwrapper.h"

enum {
  DeclarativeItemRole = Qt::UserRole
};

static const char* firstNames[] = {
  "Alice",
  "Bob",
  "Claire",
  "Denis",
  "Elaine",
  "Frank",
  "Graham",
  "Heidi",
  "Ian",
  "Jane",
  "Konrad",
  "Leane",
  "Micheal"
};

static const char* lastNames[] = {
  "Anderson",
  "Baker",
  "Callaway",
  "Donald",
  "Edelstein",
  "Farthingwood",
  "Gambon",
  "Heine",
  "Irving",
  "Jackson",
  "Kloecker",
  "Leroy",
  "McDonald"
};

static const char* emailDomains[] = {
  "example.com",
  "mydomain.com",
  "myfriendsdomain.com"
};

static const char* addressLine1s[] = {
  "123 FakeStreet",
  "321 Real McCoy Street",
  "1600 Pennsylvania Avenue",
  "10 Downing Street"
};

static const char* addressLine2s[] = {
  "Springfield",
  "Kreuzberg",
  "Prenslauerberg",
  "Mitte",
  "The Bronx",
  "Long Island"
};


static const char* cities[] = {
  "Dublin",
  "Berlin",
  "Washington",
  "New York",
  "London"
};

#define RANDOM_CHOICE(list) *( list + ( qrand() % ( sizeof list / sizeof *list ) ) )

FakeCollection::FakeCollection(const QString& colName, QObject* parent)
  : QObject(parent), m_colName(colName)
{

}

QString FakeCollection::colName() const
{
  return m_colName;
}

FakeContact::FakeContact(const QVariantList data, QObject* parent)
  : QObject(parent), m_data(data)
{

}

QString FakeContact::name() const
{
  return m_data.at(0).toString();
}

QString FakeContact::email() const
{
  return m_data.at(1).toString();
}

QString FakeContact::address() const
{
  return m_data.at(2).toString();
}

QPixmap FakeContact::image() const
{
  return m_data.at(3).value<QPixmap>();
}

QString FakeContact::phone() const
{
  return m_data.at(4).toString();
}

FakeEntityTreeModel::FakeEntityTreeModel(FakeEntityTreeModel::DataType dataType, QObject* parent)
  : QStandardItemModel(parent)
{
  if (dataType == ContactsData)
  {
    initContactData();
  } else if (dataType == CalendarData)
  {
    initCalendarData();
  }
  QHash<int, QByteArray> rns = roleNames();
  rns.insert( DeclarativeItemRole, "data" );
  setRoleNames( rns );

  qsrand(QDateTime::currentDateTime().toTime_t());
}

void FakeEntityTreeModel::initCalendarData()
{

}

void FakeEntityTreeModel::initContactData()
{
  for(int i = 1; i <= 10; ++i)
  {
    QStandardItem *item = new FakeEntityTreeItem( new FakeCollection( QString("col %1").arg(i), this) );
    appendRow(item);
    for ( int j = 0; j <= 10; ++j)
    {
      QVariantList data;
      QString firstName = RANDOM_CHOICE(firstNames);
      QString lastName = RANDOM_CHOICE(lastNames);
      data.append( QString("%1 %2").arg(firstName).arg(lastName));
      QString emaildomain = RANDOM_CHOICE(emailDomains);
      data.append( QString("%1.%2@%3").arg(firstName).arg(lastName).arg(emaildomain) );
      QString addressLine1 = RANDOM_CHOICE(addressLine1s);
      QString addressLine2 = RANDOM_CHOICE(addressLine2s);
      QString city = RANDOM_CHOICE(cities);;
      data.append( QString("%1\n%2\n%3").arg(addressLine1).arg(addressLine2).arg(city) );

      QPixmap p1(64, 64);
      p1.fill((Qt::GlobalColor)(qrand() % Qt::transparent));
      data.append(p1);
      QString phone = "+4917";
      phone.append( QString::number(qrand() % 100000000));
      data.append( phone );

      QStandardItem *contact = new FakeEntityTreeItem( new FakeContact(data, this));
      item->appendRow(contact);
    }
  }
}

FakeEntityTreeItem::FakeEntityTreeItem(QObject *wrappedObject)
  : QStandardItem(), m_containedObject(wrappedObject)
{

}


QVariant FakeEntityTreeItem::data( int role) const
{
  if (role == Qt::DisplayRole)
  {
    const FakeCollection *col = qobject_cast<const FakeCollection*>(m_containedObject);
    if (col)
      return col->colName();
    const FakeContact *contact = qobject_cast<const FakeContact*>(m_containedObject);
    if (contact)
      return contact->name();
#if 0
    const FakeEvent *event = qobject_cast<const FakeEvent*>(m_containedObject);
    if (event)
      return event->summary();
#endif
  }

  if (role == DeclarativeItemRole)
    return QVariant::fromValue( m_containedObject );
  return QStandardItem::data(role);
}


