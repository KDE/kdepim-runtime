/**
 * corewidget.h
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
#ifndef COREWIDGET_H
#define COREWIDGET_H

#include "attachment.h"

#include <qwidget.h>

namespace Komposer
{

  class CoreWidget : public QWidget
  {
    Q_OBJECT
  public:
    CoreWidget( QWidget* parent, const char* name=0 );

    virtual QString subject() const =0;
    virtual QStringList to()  const =0;
    virtual QStringList cc()  const =0;
    virtual QStringList bcc() const =0;
    virtual QString from() const =0;
    virtual QString replyTo() const =0;
    virtual AttachmentList attachments() const =0;
  };
}

#endif
