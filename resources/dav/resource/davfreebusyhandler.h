/*
    Copyright (c) 2011 Grégory Oestreicher <greg@kamago.net>

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

#ifndef DAVFREEBUSYHANDLER_H
#define DAVFREEBUSYHANDLER_H

#include <KCalCore/FreeBusy>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>

class KDateTime;
class KJob;

/**
 * @short The class that will manage DAV free-busy requests
 */
class DavFreeBusyHandler : public QObject
{
  Q_OBJECT

  public:
    /**
     * Constructs a new DavFreeBusyHandler
     */
    DavFreeBusyHandler( QObject *parent = 0 );

    /**
     * Checks if the free-busy info for @p email can be handled
     *
     * @param email The email address of the contact.
     */
    void canHandleFreeBusy( const QString &email );

    /**
     * Retrieve the free-busy info for @p email between @p start and @p end
     *
     * @param email The email address to retrieve the free-busy for
     * @param start The start of the free-busy period to report
     * @param end The end of the free-busy period to report
     */
    void retrieveFreeBusy( const QString &email, const KDateTime &start, const KDateTime &end );

  signals:
    /**
     * Emitted once we know if the free-busy info for @p email
     * can be handled or not.
     */
    void handlesFreeBusy( const QString &email, bool handles );

    /**
     * Emitted once the free-busy has been retrieved
     */
    void freeBusyRetrieved( const QString &email, bool success, const QString &freeBusy );

  private slots:
    void onPrincipalSearchJobFinished( KJob *job );
    void onRetrieveFreeBusyJobFinished( KJob *job );

  private:
    /**
     * Simple struct to track the state of requests
     */
    struct RequestTracker {
      RequestTracker()
        : handlingJobCount( 0 ), handlingJobSuccessful( false ),
          retrievalJobCount( 0 ), retrievalJobSuccessful( false )
      {
      }

      int handlingJobCount;
      bool handlingJobSuccessful;
      int retrievalJobCount;
      bool retrievalJobSuccessful;
      QMap<uint, KCalCore::FreeBusy::Ptr> resultingFreeBusy;
    };

    QMap<QString, RequestTracker> mRequestsTracker;
    QMap<QString, QStringList> mPrincipalScheduleOutbox;
    uint mNextRequestId;
};

#endif // DAVFREEBUSYHANDLER_H
