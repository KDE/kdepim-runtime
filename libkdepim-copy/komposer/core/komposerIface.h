/*
 * Copyright (C)  2004  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#ifndef KOMPOSERIFACE_H
#define KOMPOSERIFACE_H

#include <dcopobject.h>
#include <kurl.h>

namespace Komposer
{

/**
  DCOP interface for mail composer window. The address header fields are set,
  when the composer is constructed. KMailIface::openComposer() returns a
  reference to the DCOP interface of the new composer window, which provides the
  functions defined in the MailComposerIface.
*/
class KomposerIface : virtual public DCOPObject
{
  K_DCOP
k_dcop:
  /**
     Send message.

     @param how 0 for deafult method, 1 for sending now, 2 for sending later.
  */
  virtual void send(int how) = 0;

  /**
     Add url as attachment with a user-defined comment.
  */
    virtual void addAttachment( const KUrl &url, const QString &comment) = 0;

  /**
     Set message body.
  */
  virtual void setBody( const QString &body ) = 0;

  /**
     Add attachment.

     @param name Name of Attachment
     @param cte Content Transfer Encoding
     @param data Data to be attached
     @param type MIME content type
     @param subType MIME content sub type
     @param paramAttr Attribute name of parameter of content type
     @param paramValue Value of parameter of content type
     @param contDisp Content disposition
  */
  virtual void addAttachment( const QString &name,
                              const QCString &cte,
                              const QByteArray &data,
                              const QCString &type,
                              const QCString &subType,
                              const QCString &paramAttr,
                              const QString &paramValue,
                              const QCString &contDisp ) = 0;
public:
  KomposerIface( const char *name )
    : DCOPObject( name )
  {}
};

}

#endif
