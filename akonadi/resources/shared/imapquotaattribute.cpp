/*
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#include "imapquotaattribute.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <KDE/KDebug>

using namespace Akonadi;

ImapQuotaAttribute::ImapQuotaAttribute()
{
}

Akonadi::ImapQuotaAttribute::ImapQuotaAttribute( const QList<QByteArray> &roots,
                                                 const QList< QMap<QByteArray, qint64> > &limits,
                                                 const QList< QMap<QByteArray, qint64> > &usages )
  : mRoots( roots ), mLimits( limits ), mUsages( usages )
{
  Q_ASSERT( roots.size()==limits.size() );
  Q_ASSERT( roots.size()==usages.size() );
}

void Akonadi::ImapQuotaAttribute::setQuotas( const QList<QByteArray> &roots,
                                             const QList< QMap<QByteArray, qint64> > &limits,
                                             const QList< QMap<QByteArray, qint64> > &usages )
{
  Q_ASSERT( roots.size()==limits.size() );
  Q_ASSERT( roots.size()==usages.size() );

  mRoots = roots;
  mLimits = limits;
  mUsages = usages;
}

QList<QByteArray> Akonadi::ImapQuotaAttribute::roots() const
{
  return mRoots;
}

QList< QMap<QByteArray, qint64> > Akonadi::ImapQuotaAttribute::limits() const
{
  return mLimits;
}

QList< QMap<QByteArray, qint64> > Akonadi::ImapQuotaAttribute::usages() const
{
  return mUsages;
}

QByteArray ImapQuotaAttribute::type() const
{
  return "imapquota";
}

Akonadi::Attribute* ImapQuotaAttribute::clone() const
{
  return new ImapQuotaAttribute( mRoots, mLimits, mUsages );
}

QByteArray ImapQuotaAttribute::serialized() const
{
  typedef QMap<QByteArray, qint64> QuotaMap;
  QByteArray result = "";

  // First the roots list
  foreach ( const QByteArray &root, mRoots ) {
    result+=root+" % ";
  }
  result.chop( 3 );

  result+= " %%%% "; // Members separator

  // Then the limit maps list
  for ( int i=0; i<mRoots.size(); ++i ) {
    const QMap<QByteArray, qint64> limits = mLimits[i];
    foreach ( const QByteArray &key, limits.keys() ) {
      result+= key;
      result+= " % "; // We use this separator as '%' is not allowed in keys or values
      result+= QByteArray::number( limits[key] );
      result+= " %% "; // Pairs separator
    }
    result.chop( 4 );
    result+= " %%% "; // Maps separator
  }
  result.chop( 5 );

  result+= " %%%% "; // Members separator

  // Then the usage maps list
  for ( int i=0; i<mRoots.size(); ++i ) {
    const QMap<QByteArray, qint64> usages = mUsages[i];
    foreach ( const QByteArray &key, usages.keys() ) {
      result+= key;
      result+= " % "; // We use this separator as '%' is not allowed in keys or values
      result+= QByteArray::number( usages[key] );
      result+= " %% "; // Pairs separator
    }
    result.chop( 4 );
    result+= " %%% "; // Maps separator
  }
  result.chop( 5 );

  return result;
}

void ImapQuotaAttribute::deserialize( const QByteArray &data )
{
  mRoots.clear();
  mLimits.clear();
  mUsages.clear();

  // Nothing was saved.
  if ( data.trimmed().isEmpty() ) {
    return;
  }

  QString string = QString::fromUtf8(data); // QByteArray has no proper split, so we're forced to convert to QString...

  QStringList members = string.split( "%%%%" );

  // We expect exactly three members (roots, limits and usages), otherwise something is funky
  if ( members.size() != 3 ) {
    kWarning() << "We didn't find exactly three members in this quota serialization";
    return;
  }

  QStringList roots = members[0].trimmed().split( " % " );
  foreach ( const QString &root, roots ) {
    mRoots << root.trimmed().toUtf8();
  }

  QStringList allLimits = members[1].trimmed().split( "%%%" );

  foreach ( const QString &limits, allLimits ) {
    QMap<QByteArray, qint64> limitsMap;
    QStringList strLines = limits.split( "%%" );
    QList<QByteArray> lines;
    foreach ( const QString strLine, strLines ) {
      lines << strLine.trimmed().toUtf8();
    }

    foreach ( const QByteArray &line, lines ) {
      QByteArray trimmed = line.trimmed();
      int wsIndex = trimmed.indexOf( '%' );
      const QByteArray key = trimmed.mid( 0, wsIndex ).trimmed();
      const QByteArray value = trimmed.mid( wsIndex+1, line.length()-wsIndex ).trimmed();
      limitsMap[key] = value.toLongLong();
    }

    mLimits << limitsMap;
  }

  QStringList allUsages = members[2].trimmed().split( "%%%" );

  foreach ( const QString &usages, allUsages ) {
    QMap<QByteArray, qint64> usagesMap;
    QStringList strLines = usages.split( "%%" );
    QList<QByteArray> lines;
    foreach ( const QString strLine, strLines ) {
      lines << strLine.trimmed().toUtf8();
    }

    foreach ( const QByteArray &line, lines ) {
      QByteArray trimmed = line.trimmed();
      int wsIndex = trimmed.indexOf( '%' );
      const QByteArray key = trimmed.mid( 0, wsIndex ).trimmed();
      const QByteArray value = trimmed.mid( wsIndex+1, line.length()-wsIndex ).trimmed();
      usagesMap[key] = value.toLongLong();
    }

    mUsages << usagesMap;
  }
}
