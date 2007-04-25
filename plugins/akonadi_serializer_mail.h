
#ifndef __AKONADI_SERIALIZER_MAIL_H__
#define __AKONADI_SERIALIZER_MAIL_H__

#include "../libakonadi/itemserializer.h"

namespace Akonadi {

class SerializerPluginMail : public ItemSerializerPlugin
{
public:
    void deserialize( Item& item, const QString& label, const QByteArray& data ) const;
    void deserialize( Item& item, const QString& label, const QIODevice& data ) const;
    void serialize( const Item& item, const QString& label, QByteArray& data ) const;
    void serialize( const Item& item, const QString& label, QIODevice& data ) const;
};


}

#endif
