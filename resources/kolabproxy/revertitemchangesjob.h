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
#ifndef REVERTITEMCHANGESJOB
#define REVERTITEMCHANGESJOB

#include <KJob>
#include "handlermanager.h"

/**
 * Restore kolab item to the state of imapItem
 */
class RevertItemChangesJob: public KJob
{
    Q_OBJECT
public:
    RevertItemChangesJob(const Akonadi::Item &kolabItem, HandlerManager &handlerManager, QObject* parent);
    virtual void start();

private slots:
    void onImapItemFetchDone(KJob* job);
    void onItemModifyDone(KJob *job);

private:
    HandlerManager &mHandlerManager;
    const Akonadi::Item mKolabItem;
};

#endif