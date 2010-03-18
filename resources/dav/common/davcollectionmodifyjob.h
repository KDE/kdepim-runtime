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

/**
 * @short A job that modifies a DAV collection.
 *
 * This job is used to modify a property of a DAV collection
 * on the DAV server.
 */
class DavCollectionModifyJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new dav collection modify job.
     *
     * @param url The DAV url that identifies the collection.
     * @param parent The parent object.
     */
    explicit DavCollectionModifyJob( const DavUtils::DavUrl &url, QObject *parent = 0 );

    /**
     * Sets the property that shall be modified by the job.
     *
     * @param property The name of the property.
     * @param value The value of the property.
     * @param ns The XML namespace that shall be used for the property name.
     */
    void setProperty( const QString &property, const QString &value, const QString &ns = QString() );

    /**
     * Sets the property that shall be removed by the job.
     *
     * @param property The name of the property.
     * @param ns The XML namespace that shall be used for the property name.
     */
    void removeProperty( const QString &property, const QString &ns );

    /**
     * Starts the job.
     */
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
