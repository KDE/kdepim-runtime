/*
  This file is part of the Groupware/Akonadianizer integration.

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

#ifndef AKONADI_KCAL_GROUPWARE_H
#define AKONADI_KCAL_GROUPWARE_H

#include "incidencechanger.h"

#include "akonadi-kcal_next_export.h"
#include <kcalcore/icalformat.h>


namespace KCalCore {
  class Event;
  class Incidence;
}

class QString;

namespace Akonadi {
  class CalendarViewBase;
  class Calendar;
  class Calendar;
  class FreeBusyManager;
  class Item;

class AKONADI_KCAL_NEXT_EXPORT GroupwareUiDelegate
{
public:
  virtual ~GroupwareUiDelegate();

  virtual void requestIncidenceEditor( const Akonadi::Item &item ) = 0;
};

class AKONADI_KCAL_NEXT_EXPORT Groupware : public QObject
{
  Q_OBJECT
  public:

    static Groupware *create( Akonadi::Calendar *, GroupwareUiDelegate * );
    static Groupware *instance();

    FreeBusyManager *freeBusyManager();

    /** Send iCal messages after asking the user
         Returns false if the user cancels the dialog, and true if the
         user presses Yes or No.
    */
    bool sendICalMessage( QWidget *parent, KCalCore::iTIPMethod method,
                          const KCalCore::Incidence::Ptr &incidence,
                          IncidenceChanger::HowChanged action,
                          bool attendeeStatusChanged );

    /**
      Send counter proposal message.
      @param oldEvent The original event provided in the invitations.
      @param newEvent The new event as edited by the user.
    */
    void sendCounterProposal( KCalCore::Event::Ptr oldEvent, KCalCore::Event::Ptr newEvent ) const;

    // DoNotNotify is a flag indicating that the user does not want
    // updates sent back to the organizer.
    void setDoNotNotify( bool notify ) { mDoNotNotify = notify; }
    bool doNotNotify() { return mDoNotNotify; }

    bool handleInvitation( const QString& receiver, const QString& iCal,
                           const QString& type );

  private slots:
    void initialCheckForChanges();

  protected:
    Groupware( Akonadi::Calendar *, GroupwareUiDelegate * );

  private:
    static Groupware *mInstance;
    KCalCore::ICalFormat mFormat;
    Akonadi::Calendar *mCalendar;
    GroupwareUiDelegate *mDelegate;
    static FreeBusyManager *mFreeBusyManager;
    bool mDoNotNotify;
};

}

#endif
