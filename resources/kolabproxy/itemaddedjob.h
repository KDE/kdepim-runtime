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
#ifndef ITEMADDEDJOB_H
#define ITEMADDEDJOB_H
#include <KJob>
#include "kolabhandler.h"

class ItemAddedJob: public KJob
{
    Q_OBJECT
public:
    ItemAddedJob(const Akonadi::Item &kolabItem, const Akonadi::Collection &col, KolabHandler &handler, QObject* parent);
    virtual void start();
    Akonadi::Item kolabItem() const;
    Akonadi::Item imapItem() const;
private slots:
    void doStart();
    void onItemCreatedDone(KJob *job);
private:
    KolabHandler &mHandler;
    const Akonadi::Item mKolabItem;
    Akonadi::Item mImapItem;
    const Akonadi::Collection mParentCollection;
};

#endif