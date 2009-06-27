/******************************************************************************
 *
 *  File : datarfc822.h
 *  Created on Sat 20 Jun 2009 14:36:26 by Szymon Tomasz Stefanek
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

#ifndef _DATARFC822_H_
#define _DATARFC822_H_

#include <akonadi/filter/data.h>

#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KMime::Message> MessagePtr;

#include <QtCore/QVariant>

class DataRfc822 : public Akonadi::Filter::Data
{
protected:
  Akonadi::Item mItem;
  Akonadi::Collection mCollection;
  MessagePtr mMessage;
  bool mFetchedBody;

public:
  DataRfc822( const Akonadi::Item &item, const Akonadi::Collection &collection );
  virtual ~DataRfc822();

  QVariant getPropertyValue( const Akonadi::Filter::FunctionDescriptor * function, const Akonadi::Filter::DataMemberDescriptor * dataMember );
  QVariant getDataMemberValue( const Akonadi::Filter::DataMemberDescriptor * dataMember );

private:
  bool fetchHeader();
}; // class DataRfc822


#endif //!_DATARFC822_H_
