// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#ifndef KOMPOSER_ATTACHMENT_H
#define KOMPOSER_ATTACHMENT_H

#include <QString>
#include <Q3CString>
#include <Q3ValueList>

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
    Private *d;
  };
  typedef QValueList<Attachment> AttachmentList;
}

#endif
