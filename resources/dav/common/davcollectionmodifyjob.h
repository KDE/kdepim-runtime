/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

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

#ifndef DAVCOLLECTIONMODIFYJOB_H
#define DAVCOLLECTIONMODIFYJOB_H

#include "davutils.h"

#include <QtCore/QList>

#include <kjob.h>

class DavCollectionModifyJob : public KJob
{
  Q_OBJECT
  
  public:
    DavCollectionModifyJob( const DavUtils::DavUrl &url, QObject *parent = 0 );
    
    void setProperty( const QString &property, const QString &value, const QString &ns = QString() );
    void removeProperty( const QString &property, const QString &ns );
    
    virtual void start();
    
  private Q_SLOTS:
    void davJobFinished( KJob *job );
    
  private:
    DavUtils::DavUrl mUrl;
    QDomDocument mQuery;
    
    QList<QDomElement> mSetProperties;
    QList<QDomElement> mRemoveProperties;
};

#endif