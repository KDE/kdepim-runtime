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

#include "fakegrouplistmodel.h"

#include "eventgroupmodel.h"

#include <QDateTime>

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

static const char* events[] = {
  "Birthday Party",
  "Office Viewing",
  "Sprint review",
  "Sprint planning",
  "Climbing",
  "Dinner",
  "Driving Lesson",
  "Exams :(",
  "Thesis presentation"
};

static const char* descriptionLines[] = {
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam elementum risus augue.",
  "Suspendisse ullamcorper odio id leo dapibus sit amet lacinia sem luctus. In hac habitasse\n"
  "platea dictumst. Maecenas tristique ligula eget diam fermentum varius. Donec vitae molestie\n"
  "ligula. Integer in felis sed justo semper ullamcorper porta ut magna. Fusce id erat id mi\n"
  "auctor luctus. Aliquam erat volutpat. Vestibulum id luctus lectus.",
  "In hac habitasse platea dictumst. Sed eu augue at purus vestibulum volutpat sed at ante.\n"
  "Sed et leo dolor, vitae adipiscing erat. Suspendisse neque velit, faucibus vitae hendrerit\n"
  "tempus, dictum et metus. Nullam ac felis nec nisi varius tempor non nec enim. Vivamus nec\n"
  "condimentum tortor.",
  "In hac habitasse platea dictumst. Sed eu augue at purus vestibulum volutpat sed at ante. Sed\n"
  "et leo dolor, vitae adipiscing erat. Suspendisse neque velit, faucibus vitae hendrerit tempus,\n"
  "dictum et metus. Nullam ac felis nec nisi varius tempor non nec enim. Vivamus nec condimentum tortor.",
  "Nullam eu nisl non neque varius adipiscing.",
  "Curabitur quis augue in sem fermentum vestibulum. In hac\n"
  "habitasse platea dictumst. Fusce malesuada tincidunt viverra. Fusce quis volutpat nunc. Donec egestas\n"
  "convallis gravida. Nulla at ligula ipsum. Aenean condimentum, turpis eget congue aliquet, dui lorem\n"
  "convallis diam, at sodales ipsum nibh vitae lectus.",
  "Duis in ante a arcu hendrerit fermentum id vel nisi. Cras cursus, nunc eu iaculis tempus, massa velit\n"
  "luctus justo, ut ornare nibh neque sed turpis. Vivamus hendrerit tempor odio vitae condimentum. Nunc vel\n"
  "neque lectus, et consectetur erat. Curabitur ullamcorper condimentum nisl. Proin suscipit\n"
  "mollis mauris vel convallis.",
  "Mauris vel urna eu mauris vehicula accumsan in at orci. Aliquam dapibus mauris et turpis imperdiet\n"
  "fringilla congue libero dignissim. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi\n"
  "convallis adipiscing ipsum, eu fermentum lacus aliquam et. Nunc ut orci orci, ut porttitor\n"
  "mauris. Integer quis enim sem, ac interdum quam. Nullam nec massa urna.",
  "Donec mi mi, pretium ac rhoncus eu, malesuada nec nunc. Mauris feugiat sagittis neque, eget venenatis\n"
  "purus tincidunt a. Quisque turpis tortor, auctor vitae tempus id, semper vehicula augue. Nam venenatis\n"
  "tellus vel massa sagittis dictum quis nec felis"
};


#define RANDOM_CHOICE(list) *( list + ( qrand() % ( sizeof list / sizeof *list ) ) )

enum Role
{
  EventGroupRole = Qt::UserRole
};

FakeEventWrapper::FakeEventWrapper(const QVariantList data, QObject* parent)
  : QObject(parent), m_data(data)
{

}

QString FakeEventWrapper::summary() const
{
  return m_data.at(0).toString();
}

QString FakeEventWrapper::location() const
{
  return m_data.at(1).toString();
}

QString FakeEventWrapper::description() const
{
  return m_data.at(2).toString();
}

FakeEventGroupListModel::FakeEventGroupListModel(QObject* parent)
  : QAbstractListModel(parent)
{
  QHash<int, QByteArray> _roleNames = roleNames();
  _roleNames.insert(EventGroupRole, "eventsGroup");
  setRoleNames( _roleNames );
  initData();

  qsrand(QDateTime::currentDateTime().toTime_t());
}

void FakeEventGroupListModel::initData()
{
  for(int i = 1; i <= 30; ++i)
  {
    EventGroupModel *groupModel = new EventGroupModel(this);
    m_eventGroups.append(groupModel);
    if (qrand() % 3 != 0 )
      continue;

//     QStandardItem *item = new FakeEntityTreeItem( new FakeCollection( QString("col %1").arg(i), this) );
//     appendRow(item);

    for ( int j = 0; j <= 5; ++j )
    {
      if (qrand() % 2 == 0 )
        continue;
      QVariantList data;
      QString event = RANDOM_CHOICE(events);
      data.append(event);
      QString addressLine1 = RANDOM_CHOICE(addressLine2s);
      QString city = RANDOM_CHOICE(cities);
      data.append( QString("%1,%2").arg(addressLine1).arg(city));

      QString description;
      description.append( RANDOM_CHOICE(descriptionLines) );
      description.append("\n\n");
      description.append( RANDOM_CHOICE(descriptionLines) );
      description.append("\n\n");
      description.append( RANDOM_CHOICE(descriptionLines) );
      description.append("\n\n");
      description.append( RANDOM_CHOICE(descriptionLines) );
      data.append(description);
      FakeEventWrapper *fakeEvent = new FakeEventWrapper(data, this);
      m_eventGroups.last()->addIncidence(fakeEvent);
    }
  }
}

QVariant FakeEventGroupListModel::data(const QModelIndex& index, int role) const
{
  if (role == EventGroupRole)
  {
    return QVariant::fromValue( qobject_cast<QObject *>( m_eventGroups.at( index.row() ) ) );
  }
  if ( role == Qt::DisplayRole )
  {
    QAbstractItemModel *group = m_eventGroups.at( index.row() );
    int size = group->rowCount();

    if (size < 1)
      return "No events";

    return QString("%1 events").arg(size);
  }
  return QVariant();
}

void FakeEventGroupListModel::sourceRowsAboutToBeInserted()
{
  emit beginResetModel();
  m_eventGroups.clear();
}

void FakeEventGroupListModel::sourceRowsInserted()
{
  emit endResetModel();
  QModelIndex top = index(0, 0);
  QModelIndex bottom = index(rowCount() -1, 0);

  emit dataChanged(top, bottom);
}

int FakeEventGroupListModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent)
  return m_eventGroups.size();
}

