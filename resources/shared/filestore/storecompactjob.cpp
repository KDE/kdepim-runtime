/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "storecompactjob.h"

#include "session_p.h"

using namespace Akonadi::FileStore;

class StoreCompactJob::Private
{
  public:
    explicit Private( StoreCompactJob *parent )
      : mParent( parent )
    {
    }

  private:
    StoreCompactJob *mParent;
};

StoreCompactJob::StoreCompactJob( AbstractJobSession *session )
  : Job( session ), d( new Private( this ) )
{
  session->addJob( this );
}

StoreCompactJob::~StoreCompactJob()
{
  delete d;
}

bool StoreCompactJob::accept( Visitor *visitor )
{
  return visitor->visit( this );
}

#include "storecompactjob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
