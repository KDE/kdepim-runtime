/*
  This file is part of KOrganizer.

  Copyright (c) 2001,2004 Cornelius Schumacher <schumacher@kde.org>

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
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef MAILSCHEDULER_H
#define MAILSCHEDULER_H

#include "akonadi-kcal_next_export.h"

#include <kcalcore/incidence.h>

#include <QMap>
#include <kcalcore/schedulemessage.h>

namespace KCalCore {
  class ICalFormat;
  class ScheduleMessage;
}

namespace Akonadi {
  class Calendar;

  /*
    This class implements the iTIP interface using the email interface specified
    as Mail.
  */
  class AKONADI_KCAL_NEXT_EXPORT MailScheduler //: public Scheduler
  {
    public:
      explicit MailScheduler( Calendar *calendar );
      virtual ~MailScheduler();

      bool publish ( KCalCore::IncidenceBase::Ptr incidence, const QString &recipients );

      bool performTransaction( KCalCore::IncidenceBase::Ptr incidence, KCalCore::iTIPMethod method );
      bool performTransaction( KCalCore::IncidenceBase::Ptr incidence, KCalCore::iTIPMethod method, const QString &recipients );
#if 0
      QList<KCalCore::ScheduleMessage*> retrieveTransactions();

      bool deleteTransaction( KCalCore::IncidenceBase::Ptr incidence );
#endif

      /** Returns the directory where the free-busy information is stored */
      virtual QString freeBusyDir();

      /**
        Accepts the transaction. The incidence argument specifies the iCal
        component on which the transaction acts. The status is the result of
        processing a iTIP message with the current calendar and specifies the
        action to be taken for this incidence.

        @param incidence the incidence for the transaction.
        @param method iTIP transaction method to check.
        @param status scheduling status.
        @param email the email address of the person for whom this
        transaction is to be performed.
      */
      bool acceptTransaction( KCalCore::IncidenceBase::Ptr incidence, KCalCore::iTIPMethod method, KCalCore::ScheduleMessage::Status status, const QString &email );

      /** Accepts a counter proposal */
      bool acceptCounterProposal( KCalCore::Incidence::Ptr incidence );

    private:
      Calendar *mCalendar;
      KCalCore::ICalFormat *mFormat;
    #if 0
      QMap<KCalCore::IncidenceBase::Ptr , QString> mEventMap;
    #endif
  };

}

#endif
