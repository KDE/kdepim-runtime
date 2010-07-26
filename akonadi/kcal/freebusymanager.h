/*
  This file is part of the Groupware/KOrganizer integration.

  Requires the Qt and KDE widget libraries, available at no cost at
  http://www.trolltech.com and http://www.kde.org respectively

  Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt.  If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.
*/
#ifndef FREEBUSYMANAGER_H
#define FREEBUSYMANAGER_H

#include "akonadi-kcal_next_export.h"

#include <kcalcore/icalformat.h>
#include <kcalcore/freebusycache.h>
#include <kcalcore/freebusy.h>

#include <KUrl>

#include <QPointer>
#include <QByteArray>
#include <QObject>
#include <QString>

class KJob;
class QTimerEvent;

namespace KIO {
  class Job;
}

namespace Akonadi {

class Calendar;
class FreeBusyManagerPrivate;
class FreeBusyManagerStatic;

class AKONADI_KCAL_NEXT_EXPORT FreeBusyManager : public QObject, public KCalCore::FreeBusyCache
{
  Q_OBJECT
  public:
    /**
     * Returns the FreeBusyManager
     */
    static FreeBusyManager *self();

    void setCalendar( Akonadi::Calendar * );

    /**
      Publishes the owners freebusy information from the current calendar
      starting from the current date/time to current date/time + freeBusyPublishDays.
      If an upload is already in progress nothing happens.

      @see KCalPrefBase::freeBusyPublishUrl()
      @see KCalPrefBase::freeBusyPublishDays();
     */
    void publishFreeBusy( QWidget *parentWidget = 0 );

    /**
      Retrieve the freebusy information of somebody else, i.e. it will not try
      to download our own freebusy information. This method makes use of a local
      cache, if the information for a given email is already downloaded it will
      return the information from the cache.

      The call is asynchronous, a download is started in the background (if
      needed) and freeBusyRetrieved will be emitted when the download is finished.

      @see KCalPrefs::thatIsMe( const QString &email );

      @param email Address of the person for which the F/B list should be
             retrieved.
      @param forceDownload Set to true to trigger a download even when automatic
             retrieval of freebusy information is disabled in the configuration.
      @return true if a download is initiated, and false otherwise
    */
    bool retrieveFreeBusy( const QString &email, bool forceDownload,
                           QWidget *parentWidget = 0 );

    /**
      Clears the retrieval queue, i.e. all retrieval request that are not started
      yet will not start at all. The freebusy retrieval that currently is
      downloading (if one) will not be canceled.

      @see retrieveFreeBusy
     */
    void cancelRetrieval();


    /**
      Load freebusy information belonging to an email. The information is loaded
      from a local file. If the file does not exists or doesn't contain valid
      information 0 is returned. In that case the information should be retrieved
      again by calling retrieveFreeBusy.

      Implements KCalCore::FreeBusyCache::loadFreeBusy

      @param email is a QString containing a email string in the
      "FirstName LastName <emailaddress>" format.
    */
    virtual KCalCore::FreeBusy::Ptr loadFreeBusy( const QString &email );

    /**
      Save freebusy information belonging to an email.

      Implements KCalCore::FreeBusyCache::saveFreeBusy

      @param freebusy is a pointer to a valid FreeBusy instance.
      @param person is a valid Person instance.
    */
    virtual bool saveFreeBusy( const KCalCore::FreeBusy::Ptr &freebusy,
                               const KCalCore::Person::Ptr &person );

  signals:
    /**
      This signal is emitted to return results of free/busy requests.
    */
    void freeBusyRetrieved( const KCalCore::FreeBusy::Ptr &,
                            const QString &email );

  protected:
    /** React on timer events, used for delayed freebusy list uploading */
    virtual void timerEvent( QTimerEvent * );

  private:
    /**
      Creates a new FreeBusyManager, private because FreeBusyManager is a
      Singleton
     */
    FreeBusyManager();
    virtual ~FreeBusyManager();

  private:
    friend class FreeBusyManagerStatic;

    FreeBusyManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE( FreeBusyManager )
    Q_DISABLE_COPY( FreeBusyManager )
    Q_PRIVATE_SLOT( d_ptr, void checkFreeBusyUrl() )
    Q_PRIVATE_SLOT( d_ptr, void processFreeBusyDownloadResult( KJob * ) )
    Q_PRIVATE_SLOT( d_ptr, void processFreeBusyUploadResult( KJob * ) )
    Q_PRIVATE_SLOT( d_ptr, bool processRetrieveQueue() )
    Q_PRIVATE_SLOT( d_ptr, void uploadFreeBusy() )
};

}

#endif
