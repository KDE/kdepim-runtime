/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef GROUPDAVCALENDAR_H
#define GROUPDAVCALENDAR_H

#include "../common/davaccessor.h"

class KJob;

class groupdavCalendarAccessor : public davAccessor
{
  Q_OBJECT
  
  public:
    groupdavCalendarAccessor();
    virtual void retrieveCollections( const KUrl &url );
    virtual void retrieveItems( const KUrl &url );
    virtual void retrieveItem( const KUrl &url );
    
  protected:
    void getItem( const KUrl &url );
    void runItemsFetch();
    
  private slots:
    void collectionsPropfindFinished( KJob *j );
    void itemsPropfindFinished( KJob *j );
    void itemGetFinished( KJob *j );
    
  private:
    QList<QString> fetchItemsQueue;
    QSet<QString> backendChangedItems;
};

#endif