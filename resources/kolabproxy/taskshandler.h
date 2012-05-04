/*
  Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Andras Mantia <andras@kdab.net>
  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef KOLABPROXY_TASKSHANDLER_H
#define KOLABPROXY_TASKSHANDLER_H

#include "incidencehandler.h"

/**
	@author Andras Mantia <amantia@kde.org>
*/
class TasksHandler : public IncidenceHandler
{
  public:
    explicit TasksHandler( const Akonadi::Collection &imapCollection );
    virtual ~TasksHandler();
    virtual QStringList contentMimeTypes();
    virtual QString iconName() const;

  private:
    virtual KMime::Message::Ptr incidenceToMime( const KCalCore::Incidence::Ptr &incidence );

};

#endif
