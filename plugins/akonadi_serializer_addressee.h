
#ifndef __AKONADI_SERIALIZER_ADDRESSEE_H__
#define __AKONADI_SERIALIZER_ADDRESSEE_H__

#include "../libakonadi/itemserializer.h"
#include <kabc/vcardconverter.h>

namespace Akonadi {

class SerializerPluginAddresee : public ItemSerializerPlugin
{
public:
    void deserialize( Item& item, const QString& label, const QByteArray& data ) const;
    void deserialize( Item& item, const QString& label, const QIODevice& data ) const;
    void serialize( const Item& item, const QString& label, QByteArray& data ) const;
    void serialize( const Item& item, const QString& label, QIODevice& data ) const;
private:
    KABC::VCardConverter m_converter;
};


}

#endif
