/*
  Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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
#ifndef ITEMCHANGEDJOB_H
#define ITEMCHANGEDJOB_H
#include <KJob>
#include "kolabhandler.h"
#include "handlermanager.h"

class ItemChangedJob: public KJob
{
    Q_OBJECT
public:
    ItemChangedJob(const Akonadi::Item &kolabItem, HandlerManager &handler, QObject* parent);
    virtual void start();
    Akonadi::Item item() const;
private slots:
    void onKolabCollectionFetched(KJob* job);
    void onImapItemFetchDone(KJob *job);
    void onCollectionFetchDone(KJob *job);
    void onItemAddedDone(KJob *job);
    void onItemModifyDone(KJob *job);
private:
    HandlerManager &mHandlerManager;
    KolabHandler::Ptr mHandler;
    const Akonadi::Item mKolabItem;
};

#endif