/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef DAVCOLLECTIONSMULTIFETCHJOB_H
#define DAVCOLLECTIONSMULTIFETCHJOB_H

#include "davcollection.h"

#include <kjob.h>
#include <kurl.h>

class DavCollectionsMultiFetchJob : public KJob
{
  Q_OBJECT

  public:
    DavCollectionsMultiFetchJob( const KUrl::List &urls, QObject *parent = 0 );

    virtual void start();

    DavCollection::List collections() const;

  private Q_SLOTS:
    void davJobFinished( KJob* );

  private:
    KUrl::List mUrls;
    DavCollection::List mCollections;
    uint mSubJobCount;
};

#endif
