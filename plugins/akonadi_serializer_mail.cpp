
#include "akonadi_serializer_mail.h"

#include <QDebug>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

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
    m->setContent( data );
    m->parse();
    item.setPayload( boost::shared_ptr<Message>(m) );
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

    boost::shared_ptr<Message> m = item.payload< boost::shared_ptr<Message> >();
    m->assemble();
    data = m->encodedContent();
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


