/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/


#include "akonadibrowsermodel.h"

#include <kmime/kmime_message.h>

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <kcal/incidence.h>
#include <kcal/event.h>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KMime::Message> MessagePtr;
typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

class AkonadiBrowserModel::State
{
public:
  virtual ~State() {}
  QStringList m_collectionHeaders;
  QStringList m_itemHeaders;

  virtual QVariant getData( const Item &item, int column, int role ) const = 0;

};

class GenericState : public AkonadiBrowserModel::State
{
public:
  GenericState()
  {
    m_collectionHeaders << "Collection";
    m_itemHeaders << "Id" << "Remote Id" << "MimeType";
  }
  virtual ~GenericState() {}

  QVariant getData( const Item &item, int column, int role ) const
  {
    if (Qt::DisplayRole != role)
      return QVariant();
      
    switch (column)
    {
    case 0:
      return item.id();
    case 1:
      return item.remoteId();
    case 2:
      return item.mimeType();
    }
    return QVariant();
  }


};

class MailState : public AkonadiBrowserModel::State
{
public:
  MailState()
  {
    m_collectionHeaders << "Collection";
    m_itemHeaders << "Subject" << "Sender" << "Date";
  }
  virtual ~MailState() {}

  QVariant getData( const Item &item, int column, int role ) const
  {
    if (Qt::DisplayRole != role)
      return QVariant();
    
    if (!item.hasPayload<MessagePtr>())
    {
      return QVariant();
    }
    const MessagePtr mail = item.payload<MessagePtr>();
    
    switch (column)
    {
    case 0:
      return mail->subject()->asUnicodeString();
    case 1:
      return mail->from()->asUnicodeString();
    case 2:
      return mail->date()->asUnicodeString();
    }
    
    return QVariant();
  }
  
};

class ContactsState : public AkonadiBrowserModel::State
{
public:
  ContactsState()
  {
    m_collectionHeaders << "Collection";
    m_itemHeaders << "Given Name" << "Family Name" << "Email";
  }
  virtual ~ContactsState() {}

  QVariant getData( const Item &item, int column, int role ) const
  {
    if (Qt::DisplayRole != role)
      return QVariant();
    
    if ( !item.hasPayload<KABC::Addressee>() && !item.hasPayload<KABC::ContactGroup>() )
    {
      return QVariant();
    }

    if ( item.hasPayload<KABC::Addressee>() )
    {
      const KABC::Addressee addr = item.payload<KABC::Addressee>();

      switch (column)
      {
      case 0:
        return addr.givenName();
      case 1:
        return addr.familyName();
      case 2:
        return addr.preferredEmail();
      }
      return QVariant();
    }
    if ( item.hasPayload<KABC::ContactGroup>() ) {

      switch (column)
      {
      case 0:
        const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
        return group.name();
      }
      return QVariant();
    }
    return QVariant();
  }
};

class CalendarState : public AkonadiBrowserModel::State
{
public:
  CalendarState()
  {
    m_collectionHeaders << "Collection";
    m_itemHeaders << "Summary" << "DateTime start" << "DateTime End" << "Type";
  }
  virtual ~CalendarState() {}

  QVariant getData( const Item &item, int column, int role ) const
  {
    if (Qt::DisplayRole != role)
      return QVariant();
    
    if ( !item.hasPayload<IncidencePtr>() )
    {
      return QVariant();
    }
    const IncidencePtr incidence = item.payload<IncidencePtr>();
    switch (column)
    {
    case 0:
      return incidence->summary();
      break;
    case 1:
      return incidence->dtStart().toString();
      break;
    case 2:
      return incidence->dtEnd().toString();
      break;
    case 3:
      return incidence->type();
      break;
    default:
      break;
    }
    return QVariant();
  }

};

AkonadiBrowserModel::AkonadiBrowserModel( Session* session, Monitor* monitor, QObject* parent )
    : EntityTreeModel( session, monitor, parent ),
      m_itemDisplayMode( GenericMode )
{

  m_genericState = new GenericState();
  m_mailState = new MailState();
  m_contactsState = new ContactsState();
  m_calendarState = new CalendarState();
  
  m_currentState = m_genericState;
}


int AkonadiBrowserModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return qMax(m_currentState->m_collectionHeaders.size(), m_currentState->m_itemHeaders.size());
}

QVariant AkonadiBrowserModel::getData( const Item &item, int column, int role ) const
{
  QVariant var = m_currentState->getData(item, column, role);
  if (!var.isValid())
    return EntityTreeModel::getData(item, column, role);

  return var;
}

QVariant AkonadiBrowserModel::getData(const Akonadi::Collection& collection, int column, int role) const
{
  return Akonadi::EntityTreeModel::getData(collection, column, role);
}

int AkonadiBrowserModel::getNumColumns(int headerSet) const
{
  if (ItemListHeaders == headerSet)
  {
    if (m_currentState == m_calendarState)
      kDebug() << m_currentState->m_itemHeaders;
    return m_currentState->m_itemHeaders.size();
  }

  if (CollectionTreeHeaders == headerSet)
  {
    return m_currentState->m_collectionHeaders.size();
  }
  // Practically, this should never happen.
  return columnCount();
}


QVariant AkonadiBrowserModel::getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const
{
  if (orientation == Qt::Vertical)
     return EntityTreeModel::getHeaderData(section, orientation, role, headerSet);

  if ( headerSet == EntityTreeModel::CollectionTreeHeaders )
  {
    if (role == Qt::DisplayRole)
    {
      if (section >= m_currentState->m_collectionHeaders.size() )
        return QVariant();
      return m_currentState->m_collectionHeaders.at(section);
    }
  } else if (headerSet == EntityTreeModel::ItemListHeaders)
  {
    if (role == Qt::DisplayRole)
    {
      if (section >= m_currentState->m_itemHeaders.size() )
        return QVariant();
      return m_currentState->m_itemHeaders.at(section);
    }
  }
  return EntityTreeModel::getHeaderData(section, orientation, role, headerSet);
}

AkonadiBrowserModel::ItemDisplayMode AkonadiBrowserModel::itemDisplayMode() const
{
  return m_itemDisplayMode;
}

void AkonadiBrowserModel::setItemDisplayMode( AkonadiBrowserModel::ItemDisplayMode itemDisplayMode )
{
  m_itemDisplayMode = itemDisplayMode;
  switch (itemDisplayMode)
  {
  case MailMode:
    m_currentState = m_mailState;
    break;
  case ContactsMode:
    m_currentState = m_contactsState;
    break;
  case CalendarMode:
    m_currentState = m_calendarState;
    break;
  case GenericMode:
  default:
    m_currentState = m_genericState;
    break;
  }
  reset();
}


