// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
/**
 * attachment.cpp
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
#include "attachment.h"

using namespace Komposer;

class Attachment::Private
{
public:
  QString name;
  QCString cte;
  QByteArray data;
  QCString type;
  QCString subType;
  QCString paramAttr;
  QString paramValue;
  QCString contDisp;
};

Attachment::Attachment( const QString &name,
                        const QCString &cte,
                        const QByteArray &data,
                        const QCString &type,
                        const QCString &subType,
                        const QCString &paramAttr,
                        const QString &paramValue,
                        const QCString &contDisp )
  : d( new Private )
{
  d->name = name;
  d->cte = cte;
  d->data = data;
  d->type = type;
  d->subType = subType;
  d->paramAttr = paramAttr;
  d->paramValue = paramValue;
  d->contDisp = contDisp;
}

Attachment::~Attachment()
{
  delete d; d = 0;
}

QString
Attachment::name() const
{
  return d->name;
}

QCString
Attachment::cte() const
{
  return d->cte;
}

QByteArray
Attachment::data() const
{
  return d->data;
}

QCString
Attachment::type() const
{
  return d->type;
}


QCString
Attachment::subType() const
{
  return d->subType;
}

QCString
Attachment::paramAttr() const
{
  return d->paramAttr;
}

QString
Attachment::paramValue() const
{
  return d->paramValue;
}

QCString
Attachment::contentDisposition() const
{
  return d->contDisp;
}

