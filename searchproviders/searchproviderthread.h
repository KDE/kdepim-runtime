/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SEARCHPROVIDER_THREAD_H
#define AKONADI_SEARCHPROVIDER_THREAD_H

#include <QThread>

namespace Akonadi {

class SearchProviderThreadPrivate;

/**
  Base class of a search provider worker thread.
*/
class SearchProviderThread : public QThread
{
  Q_OBJECT

  public:
    enum WorkType {
      Search,
      FetchResponse
    };

    /**
      Creates a new search provider worker thread.
    */
    SearchProviderThread( int ticket, QObject *parent = 0 );

    virtual ~SearchProviderThread();
    virtual void run();

    /**
     * Returns the job ticket.
    */
    int ticket() const;

    /**
      Request fetch response generation.
    */
    void requestFetchResponse( const QList< int > uids, const QString & field );

    /**
      Returns the fetch response result after the thread has finished its work.
    */
    QStringList fetchResponse() const;

  signals:
    void finished( SearchProviderThread* thread );

  protected:
    /**
      Reimplement this method to generate the actual fetch response.
    */
    virtual QStringList generateFetchResponse( const QList<int> uids, const QString &field ) = 0;

  private slots:
    void slotFinished();

  private:
    SearchProviderThreadPrivate* const d;
};

}

#endif
