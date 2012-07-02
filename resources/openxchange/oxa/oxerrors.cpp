/*
    This file is part of oxaccess.

    Copyright (c) 2012 Marco Nelles <marco.nelles@credativ.com>

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

#include "oxerrors.h"

using namespace OXA;

OXErrors::EditErrorID OXErrors::getEditErrorID( const QString& errorText )
{
  int b1Pos = errorText.indexOf( '[' );
  int b2Pos = errorText.indexOf( ']' );
  QString errorID = errorText.mid( b1Pos+1, b2Pos-b1Pos-1 );

  bool ok;
  int eid = errorID.toInt( &ok );
  if ( !ok ) return OXErrors::EditErrorUndefined;

  switch ( eid ) {
    case 1000 : return OXErrors::ConcurrentModification;
    case 1001 : return OXErrors::ObjectNotFound;
    case 1002 : return OXErrors::NoPermissionForThisAction;
    case 1003 : return OXErrors::ConflictsDetected;
    case 1004 : return OXErrors::MissingMandatoryFields;
    case 1006 : return OXErrors::AppointmentConflicts;
    case 1500 : return OXErrors::InternalServerError;
    default : ;
  }

  return OXErrors::EditErrorUndefined;
}
