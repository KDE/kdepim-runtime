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

#ifndef AKONADI_SEARCHPROVIDER_BASE_H
#define AKONADI_SEARCHPROVIDER_BASE_H

#include <searchprovider.h>
#include <kdepim_export.h>
namespace Akonadi {

class SearchProviderThread;
class SearchProviderBasePrivate;

/**
  This is a convenience base class for search providers which provides
  threading support.

  @todo Where do we store active searches, in the control process or in
  every search provider itself?
*/
class AKONADISEARCHPROVIDER_EXPORT  SearchProviderBase : public SearchProvider
{
  Q_OBJECT

  public:
    SearchProviderBase( const QString &id );
    virtual ~SearchProviderBase();

    virtual bool addSearch( const QString &targetCollection, const QString &searchQuery, const QDBusMessage &msg );
    virtual bool removeSearch( const QString &collection );
    virtual QStringList fetchResponse( const QList<QString> uids, const QString &field, const QDBusMessage &msg );
    virtual void quit();

    /**
      Returns a worker thread. You need to reimplement this method and return an
      object derived from SearchProviderThread.
    */
    virtual SearchProviderThread* workerThread() = 0;

    /**
     * Use this method in the main function of your search provider
     * application to initialize your search provider subclass.
     *
     * \code
     *
     *   class MySearchProvider : public SearchProviderBase
     *   {
     *     ...
     *   };
     *
     *   int main( int argc, char **argv )
     *   {
     *     QCoreApplication app;
     *     ResourceBase::init<MySearchProvider>( argc, argv, "akonadi_foo_searchprovider" );
     *     return app.exec();
     *   }
     *
     * \endcode
     */
    template <typename T> static void init( int argc, char **argv, const QString &id )
    {
      new T( id );
    }

  private:
    QStringList performRequest( SearchProviderThread *thread, const QDBusMessage &msg );

  private Q_SLOTS:
    void slotThreadFinished( SearchProviderThread* thread );

  private:
    SearchProviderBasePrivate* const d;
};

}

#endif
