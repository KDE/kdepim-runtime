/**
 * attachment.h
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 */
#ifndef KOMPOSER_ATTACHMENT_H
#define KOMPOSER_ATTACHMENT_H

#include <qstring.h>
#include <qcstring.h>
#include <qvaluelist.h>

namespace Komposer
{

  class Attachment
  {
  public:
    Attachment( const QString &name,
                const QCString &cte,
                const QByteArray &data,
                const QCString &type,
                const QCString &subType,
                const QCString &paramAttr,
                const QString &paramValue,
                const QCString &contDisp );
    ~Attachment();

    QString name() const;
    QCString cte() const;
    QByteArray data() const;
    QCString type() const;
    QCString subType() const;
    QCString paramAttr() const;
    QString paramValue() const;
    QCString contentDisposition() const;

  private:
    class Private;
    Private* d;
  };
  typedef QValueList<Attachment> AttachmentList;
}

#endif
