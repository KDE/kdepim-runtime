/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "v2helpers.h"

#include "kolabdefinitions.h"

#include "kolabformatV2/kolabbase.h"
#include "kolabformatV2/journal.h"
#include "kolabformatV2/task.h"
#include "kolabformatV2/event.h"
#include "kolabformatV2/contact.h"
#include "kolabformatV2/distributionlist.h"
#include "kolabformatV2/note.h"
#include "mime/mimeutils.h"
#include "kolabformat/errorhandler.h"

#include <kcontacts/contactgroup.h>

#include <qdom.h>
#include <qbuffer.h>
#include <QDebug>
#include <akonadi/notes/noteutils.h>

namespace Kolab {

void getAttachments(KCalCore::Incidence::Ptr incidence, const QStringList &attachments, const KMime::Message::Ptr &mimeData)
{
    if (!incidence) {
        Error() << "Invalid incidence";
        return;
    }
    foreach (const QString &name, attachments) {
        QByteArray type;
        KMime::Content *content = Mime::findContentByName(mimeData, name, type);
        if (!content) { // guard against malformed events with non-existent attachments
            Warning() << "could not find attachment: "<< name.toUtf8() << type;
            continue;
        }
        const QByteArray c = content->decodedContent().toBase64();
        KCalCore::Attachment::Ptr attachment( new KCalCore::Attachment( c, QString::fromLatin1( type ) ) );
        attachment->setLabel( name );
        incidence->addAttachment(attachment);
        Debug() << "ATTACHMENT NAME" << name << type;
    }
}

static QImage getPicture(const QString &pictureAttachmentName, const KMime::Message::Ptr &data, QByteArray &type)
{
    if (!data) {
        Critical() << "empty message";
        return QImage();
    }
    KMime::Content *imgContent = Mime::findContentByName(data, pictureAttachmentName/*"kolab-picture.png"*/, type);
    if (!imgContent) {
        Warning() << "could not find picture: " << pictureAttachmentName;
        return QImage();
    }
    QByteArray imgData = imgContent->decodedContent();
    QBuffer buffer(&imgData);
    buffer.open(QIODevice::ReadOnly);
    QImage image;
    bool success = false;
    if (type == "image/jpeg") {
        success = image.load(&buffer, "JPEG");
        //FIXME I tried getting the code to interpret the picture as PNG, but the VCard implementation writes it as JPEG anyways...
//         if (success) {
//             QByteArray pic;
//             QBuffer b(&pic);
//             b.open(QIODevice::ReadWrite);
//             Q_ASSERT(image.save(&b, "PNG"));
//             b.close();
//             Debug() << pic.toBase64();
//             QBuffer b2(&pic);
//             b2.open(QIODevice::ReadOnly);
//             success = image.load(&b2, "PNG");
//             b2.close();
//             Q_ASSERT(success);
//         }
    } else {
        type = "image/png";
        success = image.load(&buffer, "PNG");
    }
    buffer.close();
    if (!success) {
        Warning() << "failed to load picture";
    }
    return image;
}

KContacts::Addressee addresseeFromKolab( const QByteArray &xmlData, const KMime::Message::Ptr &data)
{
    if (!data) {
        Critical() << "empty message";
        return KContacts::Addressee();
    }
    KContacts::Addressee addressee;
//     Debug() << "xmlData " << xmlData;
    KolabV2::Contact contact(QString::fromUtf8(xmlData));
    QByteArray type;
    const QString &pictureAttachmentName = contact.pictureAttachmentName();
    if (!pictureAttachmentName.isEmpty()) {
        const QImage &img = getPicture(pictureAttachmentName, data, type);
        contact.setPicture(img, type);
    }
    
    const QString &logoAttachmentName = contact.logoAttachmentName();
    if (!logoAttachmentName.isEmpty()) {
        contact.setLogo(getPicture(logoAttachmentName, data, type), type);
    }
    
    const QString &soundAttachmentName = contact.soundAttachmentName();
    if (!soundAttachmentName.isEmpty()) {
        QByteArray type;
        KMime::Content *content = Mime::findContentByName(data, soundAttachmentName/*"sound"*/, type);
        if (content) {
            const QByteArray &sData = content->decodedContent();
            contact.setSound(sData);
        } else {
            Warning() << "could not find sound: " << soundAttachmentName;
        }
    }
    contact.saveTo(&addressee);
    return addressee;
}

KContacts::Addressee addresseeFromKolab(const QByteArray &xmlData, QString &pictureAttachmentName, QString &logoAttachmentName, QString &soundAttachmentName)
{
    KContacts::Addressee addressee;
    KolabV2::Contact contact(QString::fromUtf8(xmlData));
    pictureAttachmentName = contact.pictureAttachmentName();
    logoAttachmentName = contact.logoAttachmentName();
    soundAttachmentName = contact.soundAttachmentName();
    contact.saveTo(&addressee);
    return addressee;
}

static QByteArray createPicture(const QImage &img, const QString &/*format*/, QString &type)
{
    QByteArray pic;
    QBuffer buffer(&pic);
    buffer.open(QIODevice::WriteOnly);
    type = "image/png";
    //FIXME it's not possible to save jpegs lossless, so we always use png. otherwise we would compress the image on every write.
//     if (format == "image/jpeg") {
//         type = "image/jpeg";
//         img.save(&buffer, "JPEG");
//     } else {
        img.save(&buffer, "PNG");
//     }
    buffer.close();
    return pic;
}

KMime::Message::Ptr contactToKolabFormat(const KolabV2::Contact& contact, const QString &productId)
{
    KMime::Message::Ptr message = Mime::createMessage( QByteArray(KOLAB_TYPE_CONTACT), false, productId.toLatin1() );
    if (!message) {
        Critical() << "empty message";
        return KMime::Message::Ptr();
    }
    message->subject()->fromUnicodeString( contact.uid(), "utf-8" );
    message->from()->fromUnicodeString( contact.fullEmail(), "utf-8" );
    
    KMime::Content* content = Mime::createMainPart( KOLAB_TYPE_CONTACT, contact.saveXML().toUtf8() );
    message->addContent( content );
    
    if ( !contact.picture().isNull() ) {
        QString type;
        const QByteArray &pic = createPicture(contact.picture(), contact.pictureFormat(), type);
        content = Mime::createAttachmentPart(QByteArray(), type.toLatin1(), /*"kolab-picture.png"*/contact.pictureAttachmentName(), pic );
        message->addContent(content);
    }
    
    if ( !contact.logo().isNull() ) {
        QString type;
        const QByteArray &pic = createPicture(contact.logo(), contact.logoFormat(), type);
        content = Mime::createAttachmentPart(QByteArray(), type.toLatin1(), /*"kolab-logo.png"*/contact.logoAttachmentName(), pic );
        message->addContent(content);
    }
    
    if ( !contact.sound().isEmpty() ) {
        content = Mime::createAttachmentPart(QByteArray(), "audio/unknown", /*"sound"*/contact.soundAttachmentName(), contact.sound() );
        message->addContent(content);
    }
    
    message->assemble();
    return message;
}

KContacts::ContactGroup contactGroupFromKolab(const QByteArray &xmlData)
{
    KContacts::ContactGroup contactGroup;
    //     qDebug() << "xmlData " << xmlData;
    KolabV2::DistributionList distList(QString::fromUtf8(xmlData));
    distList.saveTo(&contactGroup);
    return contactGroup;
}

KMime::Message::Ptr distListToKolabFormat(const KolabV2::DistributionList& distList, const QString &productId)
{    
    KMime::Message::Ptr message = Mime::createMessage( KOLAB_TYPE_DISTLIST_V2, false, productId.toLatin1() );
    if (!message) {
        Critical() << "empty message";
        return KMime::Message::Ptr();
    }
    message->subject()->fromUnicodeString( distList.uid(), "utf-8" );
    message->from()->fromUnicodeString( distList.uid(), "utf-8" );
    
    KMime::Content* content = Mime::createMainPart( KOLAB_TYPE_DISTLIST_V2, distList.saveXML().toUtf8() );
    message->addContent( content );
    
    message->assemble();
    return message;
}

KMime::Message::Ptr noteFromKolab(const QByteArray &xmlData, const KDateTime &creationDate)
{
    KolabV2::Note j;
    if ( !j.load( xmlData ) ) {
        Warning() << "failed to read note";
        return KMime::Message::Ptr();
    }
    
    Akonadi::NoteUtils::NoteMessageWrapper note;
    note.setTitle(j.summary());
    note.setText(j.body().toUtf8());
    note.setFrom("kolab@kde4");
    note.setCreationDate(creationDate.dateTime());
    return note.message();
}

KMime::Message::Ptr noteToKolab(const KMime::Message::Ptr& msg, const QString &productId)
{
    if (!msg) {
        Critical() << "empty message";
        return KMime::Message::Ptr();
    }
    Akonadi::NoteUtils::NoteMessageWrapper note(msg);
    return Mime::createMessage(note.title(), KOLAB_TYPE_NOTE, KOLAB_TYPE_NOTE, noteToKolabXML(msg), false, productId);
}

QByteArray noteToKolabXML(const KMime::Message::Ptr& msg)
{
    if (!msg) {
        Critical() << "empty message";
        return QByteArray();
    }
    Akonadi::NoteUtils::NoteMessageWrapper note(msg);
    KolabV2::Note j;
    j.setSummary( note.title() );
    j.setBody( note.text() );
    return j.saveXML().toUtf8();
}

QStringList readLegacyDictionaryConfiguration(const QByteArray &xmlData, QString &language)
{
    QStringList dictionary;
    const QDomDocument xmlDoc = KolabV2::KolabBase::loadDocument( QString::fromUtf8(xmlData) ); //TODO extract function from V2 format
    if ( xmlDoc.isNull() ) {
        Error() << "Failed to read the xml document";
        return QStringList();
    }

    QDomElement top = xmlDoc.documentElement();

    if ( top.tagName() != "configuration" ) {
        qWarning( "XML error: Top tag was %s instead of the expected configuration",
                top.tagName().toAscii().data() );
        return QStringList();
    }

    for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
        if ( n.isComment() || !n.isElement() )
            continue;
        QDomElement e = n.toElement();
        if (e.tagName() == "language") {
            language = e.text();
        } else if (e.tagName() == "e") {
            dictionary.append(e.text());
        }
    }
    return dictionary;
}

}
