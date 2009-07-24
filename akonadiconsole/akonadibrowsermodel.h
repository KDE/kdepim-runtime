/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADIBROWSERMODEL_H
#define AKONADIBROWSERMODEL_H

#include <akonadi/entitytreemodel.h>

using namespace Akonadi;

class AkonadiBrowserModel : public EntityTreeModel
{
  Q_OBJECT
public:
  AkonadiBrowserModel( Session* session, Monitor* monitor, QObject* parent = 0 );

  enum ItemDisplayMode
  {
    GenericMode,
    MailMode,
    ContactsMode,
    CalendarMode
  };
  
  void setItemDisplayMode( ItemDisplayMode itemDisplayMode );
  ItemDisplayMode itemDisplayMode() const;

  virtual QVariant getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const;

  virtual QVariant getData(const Item &item, int column, int role) const;
  virtual QVariant getData(const Collection &collection, int column, int role) const;

  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

  virtual int getColumnCount(int headerSet) const;

  class State;
private:
  State *m_currentState;
  State *m_genericState;
  State *m_mailState;
  State *m_contactsState;
  State *m_calendarState;
  
  ItemDisplayMode m_itemDisplayMode;

};

#endif
