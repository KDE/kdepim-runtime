/*
    This file is part of libkdepim.

    Copyright (c) 2003 Daniel Molkentin <molkentin@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef MAILTRANSPORTSERVICEIFACE_H
#define MAILTRANSPORTSERVICEIFACE_H

#include <dcopobject.h>
#include <dcopref.h>
#include <kurl.h>
#include <qstring.h>
#include <qcstring.h>

#include <kdepimmacros.h>

namespace KPim {

#define MailTransportServiceIface KDE_EXPORT MailTransportServiceIface
  class MailTransportServiceIface : virtual public DCOPObject
#undef MailTransportServiceIface
  {
    K_DCOP
      
    k_dcop:
      /**
       * This method will compose a message and send it using the mailers
       * preferred transport. The mimetype of the attachments passed is
       * determined using mime magic.
       *
       * @return true when the message was send successfully, false on failure.
       **/
      virtual bool sendMessage( const QString& from, const QString& to, 
                                const QString& cc, const QString& bcc,
                                const QString& subject, const QString& body, 
                                const KURL::List& attachments ) = 0;

      /**
       * This method basically behaves like the one above, but takes only one
       * attachment as QByteArray. This is useful if you want to attach simple
       * text files (e.g. a vCalendar). The mimetype is determined using
       * mime magic.
       *
       * @return true when the message was send successfully, false on failure.
       **/
      virtual bool sendMessage( const QString& from, const QString& to, 
                                const QString& cc, const QString& bcc,
                                const QString& subject, const QString& body, 
                                const QByteArray& attachment ) = 0;

    k_dcop_hidden:
      /**
       * This method is deprecated. Use the corresponding method with the
       * additional parameter from instead.
       **/
      // FIXME KDE 4.0: Remove this.
      virtual bool sendMessage( const QString& to, 
                                const QString& cc, const QString& bcc,
                                const QString& subject, const QString& body, 
                                const KURL::List& attachments ) = 0;
			       
      /**
       * This method is deprecated. Use the corresponding method with the
       * additional parameter from instead.
       **/
      // FIXME KDE 4.0: Remove this.
      virtual bool sendMessage( const QString& to,
                                const QString& cc, const QString& bcc,
                                const QString& subject, const QString& body, 
                                const QByteArray& attachment ) = 0;

  };

}

#endif // MAILTRANSPORTSERVICEIFACE_H

