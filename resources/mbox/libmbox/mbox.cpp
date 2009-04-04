/*
    Copyright (c) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "mbox.h"

#include <klocalizedstring.h>
#include <QtCore/QFileInfo>

class MBox::Private
{
  public:
    Private(const QString &mboxFile) : mMboxFile(mboxFile)
    {}

    QString mMboxFile;
};

MBox::MBox(const QString &mboxFile) : d(new Private(mboxFile))
{
}

MBox::~MBox()
{
  delete d;
}

bool MBox::isValid(QString &errorMsg) const
{
  QFileInfo info(d->mMboxFile);

  if (!info.isFile()) {
    errorMsg = i18n("%1 is not a file.").arg(d->mMboxFile);
    return false;
  }

  if (!info.exists()) {
    errorMsg = i18n("%1 does not exist").arg(d->mMboxFile);
    return false;
  }

  return true;
}
