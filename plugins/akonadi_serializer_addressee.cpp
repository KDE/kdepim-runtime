
#include "akonadi_serializer_addressee.h"

using namespace Akonadi;

void SerializerPluginAddresee::deserialize( Item& item, const QString& label, const QByteArray& data ) const
{
}


void SerializerPluginAddresee::deserialize( Item& item, const QString& label, const QIODevice& data ) const
{
}


void SerializerPluginAddresee::serialize( const Item& item, const QString& label, QByteArray& data ) const
{
}


void SerializerPluginAddresee::serialize( const Item& item, const QString& label, QIODevice& data ) const
{
}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
libakonadi_serializer_addressee_create_item_serializer_plugin() {
  return new SerializerPluginAddresee();
}


