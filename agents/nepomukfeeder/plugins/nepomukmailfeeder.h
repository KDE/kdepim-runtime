/*
    Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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


#ifndef NEPOMUKMAILFEEDER_H
#define NEPOMUKMAILFEEDER_H

#include <nepomukfeederplugin.h>

#include <kmime/kmime_headers.h>
#include <kmime/kmime_message.h>

namespace Akonadi {

class NepomukMailFeeder: public NepomukFeederPlugin
{
  Q_OBJECT
  Q_INTERFACES( Akonadi::NepomukFeederPlugin )
public:
  NepomukMailFeeder(QObject *parent, const QVariantList &);
  virtual void updateItem(const Akonadi::Item& item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph);

private:
  QList<QUrl> extractContactsFromMailboxes( const KMime::Types::Mailbox::List& mbs, Nepomuk::SimpleResourceGraph& graph );
  void addTranslatedTag( const char* tagName, const QString &tagLabel, const QString &icon , Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph);

  void processContent( const KMime::Message::Ptr &msg, const Akonadi::Item &item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph);
  void processFlags( const Akonadi::Item::Flags &flags, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph);
  void processHeaders( const KMime::Message::Ptr &msg, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph);
  void processPart( KMime::Content *content, const Akonadi::Item &item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph );

  KMime::Content *m_mainBodyPart;
};

}

#endif //NEPOMUKMAILFEEDER_H
