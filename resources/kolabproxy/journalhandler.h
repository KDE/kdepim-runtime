/*
    Copyright (C) 2009 Klar?lvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Copyright (c) 2009 Andras Mantia <andras@kdab.net>

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

#ifndef JOURNALHANDLER_H
#define JOURNALHANDLER_H

#include "incidencehandler.h"
#include <kcalcore/journal.h>

/**
	@author Andras Mantia <amantia@kde.org>
*/
class JournalHandler : public IncidenceHandler {
public:
  JournalHandler();
  virtual ~JournalHandler();

  virtual QStringList contentMimeTypes();
  virtual QString iconName() const;

private:
  virtual QByteArray incidenceToXml( const KCalCore::Incidence::Ptr &incidence);
  virtual KCalCore::Incidence::Ptr incidenceFromKolab(const KMime::Message::Ptr &data);
  KCalCore::Journal::Ptr journalFromKolab(const KMime::Message::Ptr &data);
};

#endif
