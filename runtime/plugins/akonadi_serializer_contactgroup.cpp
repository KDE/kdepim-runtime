/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "akonadi_serializer_contactgroup.h"

#include <akonadi/abstractdifferencesreporter.h>
#include <akonadi/contact/contactgroupexpandjob.h>
#include <akonadi/item.h>
#include <akonadi/kabc/contactparts.h>

#include <kabc/contactgroup.h>
#include <kabc/contactgrouptool.h>
#include <klocale.h>

#include <QtCore/qplugin.h>

using namespace Akonadi;

//// ItemSerializerPlugin interface

bool SerializerPluginContactGroup::deserialize( Item& item, const QByteArray& label, QIODevice& data, int version )
{
  Q_UNUSED( label );
  Q_UNUSED( version );

  KABC::ContactGroup contactGroup;

  if ( !KABC::ContactGroupTool::convertFromXml( &data, contactGroup ) ){
    // TODO: error reporting
    return false;
  }

  item.setPayload<KABC::ContactGroup>( contactGroup );

  return true;
}

void SerializerPluginContactGroup::serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version )
{
  Q_UNUSED( label );
  Q_UNUSED( version );

  if ( !item.hasPayload<KABC::ContactGroup>() )
    return;

  KABC::ContactGroupTool::convertToXml( item.payload<KABC::ContactGroup>(), &data );
}

//// DifferencesAlgorithmInterface interface

static bool compareString( const QString &left, const QString &right )
{
  if ( left.isEmpty() && right.isEmpty() )
    return true;
  else
    return left == right;
}

static QString toString( const KABC::Addressee &contact )
{
  return contact.fullEmail();
}

template <class T>
static void compareList( AbstractDifferencesReporter *reporter, const QString &id, const QList<T> &left, const QList<T> &right )
{
  for ( int i = 0; i < left.count(); ++i ) {
    if ( !right.contains( left[ i ] )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalLeftMode, id, toString( left[ i ] ), QString() );
  }

  for ( int i = 0; i < right.count(); ++i ) {
    if ( !left.contains( right[ i ] )  )
      reporter->addProperty( AbstractDifferencesReporter::AdditionalRightMode, id, QString(), toString( right[ i ] ) );
  }
}

void SerializerPluginContactGroup::compare( Akonadi::AbstractDifferencesReporter *reporter,
                                            const Akonadi::Item &leftItem,
                                            const Akonadi::Item &rightItem )
{
  Q_ASSERT( reporter );
  Q_ASSERT( leftItem.hasPayload<KABC::ContactGroup>() );
  Q_ASSERT( rightItem.hasPayload<KABC::ContactGroup>() );

  reporter->setLeftPropertyValueTitle( i18n( "Changed Contact Group" ) );
  reporter->setRightPropertyValueTitle( i18n( "Conflicting Contact Group" ) );

  const KABC::ContactGroup leftContactGroup = leftItem.payload<KABC::ContactGroup>();
  const KABC::ContactGroup rightContactGroup = rightItem.payload<KABC::ContactGroup>();

  if ( !compareString( leftContactGroup.name(), rightContactGroup.name() ) )
    reporter->addProperty( AbstractDifferencesReporter::ConflictMode, i18n( "Name" ),
                           leftContactGroup.name(), rightContactGroup.name() );

  // using job->exec() is ok here, not a hot path
  Akonadi::ContactGroupExpandJob *leftJob = new Akonadi::ContactGroupExpandJob( leftContactGroup );
  leftJob->exec();

  Akonadi::ContactGroupExpandJob *rightJob = new Akonadi::ContactGroupExpandJob( rightContactGroup );
  rightJob->exec();

  compareList( reporter, i18n( "Member" ), leftJob->contacts(), rightJob->contacts() );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_contactgroup, Akonadi::SerializerPluginContactGroup )

#include "akonadi_serializer_contactgroup.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
