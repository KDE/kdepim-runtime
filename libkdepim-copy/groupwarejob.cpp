/*
    This file is part of kdepim.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

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

#include "groupwarejob.h"

#include <kio/job.h>
#include <kdebug.h>

using namespace KIO;

KIO::TransferJob *GroupwareJob::getCalendar( const KURL &u )
{
  KURL url = u;
  url.setPath( "/calendar/" );
  
  kdDebug() << "GroupwareJob::getCalendar(): URL: " << url << endl;
  
  return KIO::get( url, false, false );
}

KIO::TransferJob *GroupwareJob::getAddressBook( const KURL &u )
{
  KURL url = u;
  url.setPath( "/addressbook/" );
  
  kdDebug() << "GroupwareJob::getAddressBook(): URL: " << url << endl;
  
  return KIO::get( url, false, false );
}
