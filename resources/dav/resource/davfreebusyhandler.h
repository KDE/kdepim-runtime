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

#include <KCalendarCore/FreeBusy>

#include <QMap>
#include <QObject>
#include <QString>

class QDateTime;
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
    explicit DavFreeBusyHandler(QObject *parent = nullptr);

    /**
     * Checks if the free-busy info for @p email can be handled
     *
     * @param email The email address of the contact.
     */
    void canHandleFreeBusy(const QString &email);

    /**
     * Retrieve the free-busy info for @p email between @p start and @p end
     *
     * @param email The email address to retrieve the free-busy for
     * @param start The start of the free-busy period to report
     * @param end The end of the free-busy period to report
     */
    void retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end);

Q_SIGNALS:
    /**
     * Emitted once we know if the free-busy info for @p email
     * can be handled or not.
     */
    void handlesFreeBusy(const QString &email, bool handles);

    /**
     * Emitted once the free-busy has been retrieved
     */
    void freeBusyRetrieved(const QString &email, const QString &freeBusy, bool success, const QString &errorText);

private:
    void onPrincipalSearchJobFinished(KJob *job);
    void onRetrieveFreeBusyJobFinished(KJob *job);
    /**
     * Simple struct to track the state of requests
     */
    struct RequestTracker {
        RequestTracker()
        {
        }

        int handlingJobCount = 0;
        bool handlingJobSuccessful = false;
        int retrievalJobCount = 0;
        bool retrievalJobSuccessful = false;
        QMap<uint, KCalendarCore::FreeBusy::Ptr> resultingFreeBusy;
    };

    QMap<QString, RequestTracker> mRequestsTracker;
    QMap<QString, QStringList> mPrincipalScheduleOutbox;
    uint mNextRequestId = 0;
};

#endif // DAVFREEBUSYHANDLER_H
