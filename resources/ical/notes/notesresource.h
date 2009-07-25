/*
    Copyright (c) 2009 David Jarvie <djarvie@kde.org>

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

#ifndef NOTESRESOURCE_H
#define NOTESRESOURCE_H

#include "icalresource.h"

class NotesResource : public ICalResource
{
  Q_OBJECT

  public:
    NotesResource( const QString &id );
    ~NotesResource();

  protected:
    /**
      Returns the Akonadi specific @c text/calendar sub MIME type of the given @p incidence.
    */
    virtual QString mimeType( KCal::IncidenceBase *incidence );

    /**
      Returns a list of all calendar component sub MIME types.
     */
    virtual QStringList allMimeTypes() const;
};

#endif
