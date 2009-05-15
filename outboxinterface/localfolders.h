/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef OUTBOXINTERFACE_LOCALFOLDERS_H
#define OUTBOXINTERFACE_LOCALFOLDERS_H

#include <QObject>

#include <outboxinterface/outboxinterface_export.h>

class KJob;

namespace Akonadi {
  class Collection;
}


namespace OutboxInterface {


class LocalFoldersPrivate;


/**
  Takes care of creating the Outbox and Sent-Mail collections for the mail
  dispatcher agent.

  The first time it is used, this class:
  1) creates a new maildir resource;
  2) creates 'outbox' and 'sent-mail' collections and assigns them to that
     resource;
  3) fetches those collections.
  After that, the collections are easily accessible through outbox() and
  sentMail().

*/
class OUTBOXINTERFACE_EXPORT LocalFolders : public QObject
{
  Q_OBJECT

  public:
    /**
      Returns the LocalFolders instance.  When this is first called, it
      reads the config and creates / fetches the collections.

      TODO: perhaps do this explicitly with prepare() or something?
    */
    static LocalFolders *self();

    bool isReady() const;

  public Q_SLOTS:
    /**
      Returns the outbox collection.
    */
    Akonadi::Collection outbox() const;

    /**
      Returns the sent-mail collection.
    */
    Akonadi::Collection sentMail() const;

  Q_SIGNALS:
    /**
      Emitted when the outbox and sent-mail collections have been created and
      are ready to be used via outbox() and sentMail().
    */
    void foldersReady();

  private:
    friend class LocalFoldersPrivate;

    // singleton class; the only instance resides in sInstance->instance
    LocalFolders( LocalFoldersPrivate *dd );

    LocalFoldersPrivate *const d;

    Q_PRIVATE_SLOT( d, void createResourceIfNeeded() )
    Q_PRIVATE_SLOT( d, void resourceCreateResult( KJob * ) )
    Q_PRIVATE_SLOT( d, void collectionCreateResult( KJob * ) )
    Q_PRIVATE_SLOT( d, void collectionFetchResult( KJob * ) )

};


}


#endif
