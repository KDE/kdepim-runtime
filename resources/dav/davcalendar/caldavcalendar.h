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

#ifndef CALDAVCALENDAR_H
#define CALDAVCALENDAR_H

#include <QMap>
#include <QStringList>

#include "../common/davaccessor.h"

class KJob;

class caldavCalendarAccessor : public davAccessor
{
  Q_OBJECT
  
  public:
    caldavCalendarAccessor();
    virtual void retrieveCollections( const KUrl &url );
    virtual void retrieveItems( const KUrl &url );
    virtual void retrieveItem( const KUrl &url );
    
  protected:
    void runItemsFetch( const QString &collectionUrl );
    
  private slots:
    void collectionsPropfindFinished( KJob *j );
    void itemsReportFinished( KJob *j );
    void multigetFinished( KJob *j );
    
  private:
    QMap<QString, QStringList> fetchItemsQueue; // collection url, items urls
    int runningQueries;
};

#endif