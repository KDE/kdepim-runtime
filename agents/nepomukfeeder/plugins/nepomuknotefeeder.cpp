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

#include "nepomuknotefeeder.h"

// ontology includes
#include <nepomuk/nfo.h>
#include <Soprano/Vocabulary/NAO>
#include <Nepomuk/Vocabulary/NIE>

#include <KDebug>
#include <KMime/Message>

#include <akonadi/notes/noteutils.h>
#include <nepomukfeederutils.h>

#include <kexportplugin.h>
#include <kpluginfactory.h>

using namespace Nepomuk;


namespace Akonadi {

void NepomukNoteFeeder::updateItem(const Akonadi::Item& item, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph)
{
    //kDebug() << item.id();
    Q_ASSERT(item.hasPayload());
    if ( item.hasPayload<KMime::Message::Ptr>() ) {
        Akonadi::NoteUtils::NoteMessageWrapper note(item.payload<KMime::Message::Ptr>());
        res.addType( Nepomuk::Vocabulary::NFO::HtmlDocument() );

        NepomukFeederUtils::setIcon( Akonadi::NoteUtils::noteIconName(), res, graph );

        res.setProperty( Vocabulary::NIE::title(), note.title() );

        if ( !note.text().isEmpty() ) {
            res.setProperty( Vocabulary::NIE::description(), note.text() );
            res.setProperty( Vocabulary::NIE::plainTextContent(), note.toPlainText() );
        }
        //kDebug() << "updated item " << item.url() << note.title();

        res.setProperty( Soprano::Vocabulary::NAO::prefLabel(), note.title() );

    } else {
        kWarning() << "Got item without known payload. Mimetype:" << item.mimeType()
                << "Id:" << item.id() << item.payloadData();
    }
}

K_PLUGIN_FACTORY(factory, registerPlugin<NepomukNoteFeeder>();)      
K_EXPORT_PLUGIN(factory("akonadi_nepomuk_note_feeder"))

}
