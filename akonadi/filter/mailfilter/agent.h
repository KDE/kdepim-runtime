/****************************************************************************** * *
 *  File : agent.h
 *  Created on Sat 16 May 2009 14:24:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Mail Filtering Agent
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _AKONADI_MAILFILTER_AGENT_H_
#define _AKONADI_MAILFILTER_AGENT_H_

#include <akonadi/agentbase.h>
#include <akonadi/attribute.h>

#include <QList>

class MailFilterAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT
public:
  MailFilterAgent( const QString &id );
  ~MailFilterAgent();

/*
protected:
  virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
  virtual void itemRemoved( const Akonadi::Item &item );
  virtual void collectionChanged( const Akonadi::Collection &collection );
*/
};

#endif //!_AKONADI_MAILFILTER_AGENT_H_
