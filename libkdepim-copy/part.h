/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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

#ifndef KPIM_PART_H
#define KPIM_PART_H

#include <kparts/part.h>

namespace KPIM {

/**
  This special part should be used as base class for all applications
  which shall be integrated in Kontact.

  WARNING: This class is not open for public use yet. It *will* change
           and will be marked as stable as soon as we feel confident
           about the API. We mean it! If you have questions or want to
           use it nevertheless, please contact kde-pim@kde.org.
 */
class Part : public KParts::ReadOnlyPart
{
  Q_OBJECT

  public:
    Part( QObject *parent, const char *name );
    virtual ~Part();

  signals:
    /**
      Emit this signal whenever the plugin with this part shall be
      raised inside Kontact.
     */
    void raise();
};

}

#endif
