/*
    This file is part of libkdepim.

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef KABC_RESOURCENULL_H
#define KABC_RESOURCENULL_H

#include <kabc/resource.h>

namespace KABC {

/**
  This resource does nothing.
*/
class ResourceNull : public Resource
{
  public:
    ResourceNull( const KConfig *cfg ) : Resource( cfg ) {}
    ResourceNull() : Resource() {}
    virtual ~ResourceNull() {}
  
    Ticket *requestSaveTicket() { return 0; }
    void releaseSaveTicket( Ticket * ) {}
    bool load() { return false; }
    bool save( Ticket * ) { return false; }
};

}

#endif
