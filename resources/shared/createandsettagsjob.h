/*
 *  Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */
#ifndef CREATEANDSETTAGSJOB_H
#define CREATEANDSETTAGSJOB_H

#include <KJob>
#include <akonadi/item.h>
#include <akonadi/tag.h>

class CreateAndSetTagsJob : public KJob
{
  Q_OBJECT
public:
  explicit CreateAndSetTagsJob(const Akonadi::Item &item, const Akonadi::Tag::List &tags, QObject* parent = 0);

  virtual void start();

private Q_SLOTS:
  void onCreateDone(KJob*);
  void onModifyDone(KJob*);

private:
  Akonadi::Item mItem;
  Akonadi::Tag::List mTags;
  Akonadi::Tag::List mCreatedTags;
  int mCount;
};

#endif
