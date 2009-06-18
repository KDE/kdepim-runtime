/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "idarbiterbase.h"

IdArbiterBase::~IdArbiterBase()
{
}

QString IdArbiterBase::arbitrateOriginalId( const QString &originalId )
{
  const IdSet arbitratedIds = mapToArbitratedIds( originalId );
  QString arbitratedId;
  if ( arbitratedIds.contains( originalId ) ) {
    arbitratedId = createArbitratedId();
  } else {
    arbitratedId = originalId;
  }

  mOriginalToArbitrated[ originalId ].insert( arbitratedId );
  mArbitratedToOriginal.insert( arbitratedId, originalId );

  return arbitratedId;
}

QString IdArbiterBase::mapToOriginalId( const QString &arbitratedId ) const
{
  const IdMapping::const_iterator findIt = mArbitratedToOriginal.constFind( arbitratedId );
  if ( findIt != mArbitratedToOriginal.constEnd() ) {
    return findIt.value();
  }

  return QString();
}

void IdArbiterBase::clear()
{
  mOriginalToArbitrated.clear();
  mArbitratedToOriginal.clear();
}

QString IdArbiterBase::removeArbitratedId( const QString &arbitratedId )
{
  IdMapping::iterator findIt = mArbitratedToOriginal.find( arbitratedId );
  if ( findIt != mArbitratedToOriginal.end() ) {
    const QString orignalId = findIt.value();

    IdSetMapping::iterator origIt = mOriginalToArbitrated.find( orignalId );
    origIt.value().remove( arbitratedId );
    if ( origIt.value().isEmpty() ) {
      mOriginalToArbitrated.erase( origIt );
    }

    mArbitratedToOriginal.erase( findIt );
    return orignalId;
  }

  return QString();
}

IdArbiterBase::IdSet IdArbiterBase::mapToArbitratedIds( const QString &originalId ) const
{
  const IdSetMapping::const_iterator findIt = mOriginalToArbitrated.constFind( originalId );
  if ( findIt != mOriginalToArbitrated.constEnd() ) {
    return findIt.value();
  }

  return IdSet();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
