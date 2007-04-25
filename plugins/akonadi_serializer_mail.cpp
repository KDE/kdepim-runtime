
#include "akonadi_serializer_mail.h"

#include <QDebug>
#include <kmime/kmime_message.h>

#include "../libakonadi/item.h"

using namespace Akonadi;
using namespace KMime;

void SerializerPluginMail::deserialize( Item& item, const QString& label, const QByteArray& data ) const
{
    if ( label != "RFC822" ) {
      item.addPart( label, data );
      return;
    }
    if ( item.mimeType() != QString::fromLatin1("message/rfc822") ) {
        //throw ItemSerializerException();
        return;
    }

    Message *m = new  Message();

}


void SerializerPluginMail::deserialize( Item& item, const QString& label, const QIODevice& data ) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}


void SerializerPluginMail::serialize( const Item& item, const QString& label, QByteArray& data ) const
{
    if ( label != "RFC822" )
      return;
}


void SerializerPluginMail::serialize( const Item& item, const QString& label, QIODevice& data ) const
{
  Q_UNUSED( item );
  Q_UNUSED( label );
  Q_UNUSED( data );
}

extern "C"
KDE_EXPORT Akonadi::ItemSerializerPlugin *
libakonadi_serializer_mail_create_item_serializer_plugin() {
  return new SerializerPluginMail();
}


