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


#include "kolabobject.h"
#include "v2helpers.h"
#include "kolabdefinitions.h"
#include "errorhandler.h"
#include "libkolab-version.h"

#include "kolabbase.h"
#include <kolabformatV2/journal.h>
#include <kolabformatV2/task.h>
#include <kolabformatV2/event.h>
#include <kolabformatV2/contact.h>
#include <kolabformatV2/distributionlist.h>
#include <kolabformatV2/note.h>
#include <mime/mimeutils.h>
#include <conversion/kcalconversion.h>
#include <conversion/kabcconversion.h>
#include <conversion/kolabconversion.h>
#include <conversion/commonconversion.h>
#include <akonadi/notes/noteutils.h>
#include <kolabformat/mimeobject.h>
#include <kolabformat.h>


namespace Kolab {


static inline QString eventKolabType() { return QString::fromLatin1(KOLAB_TYPE_EVENT); };
static inline QString todoKolabType() { return QString::fromLatin1(KOLAB_TYPE_TASK); };
static inline QString journalKolabType() { return QString::fromLatin1(KOLAB_TYPE_JOURNAL); };
static inline QString contactKolabType() { return QString::fromLatin1(KOLAB_TYPE_CONTACT); };
static inline QString distlistKolabType() { return QString::fromLatin1(KOLAB_TYPE_DISTLIST); }
static inline QString distlistKolabTypeCompat() { return QString::fromLatin1(KOLAB_TYPE_DISTLIST_V2); }
static inline QString noteKolabType() { return QString::fromLatin1(KOLAB_TYPE_NOTE); }
static inline QString configurationKolabType() { return QString::fromLatin1(KOLAB_TYPE_CONFIGURATION); }
static inline QString dictKolabType() { return QString::fromLatin1(KOLAB_TYPE_DICT); }
static inline QString freebusyKolabType() { return QString::fromLatin1(KOLAB_TYPE_FREEBUSY); }
static inline QString relationKolabType() { return QString::fromLatin1(KOLAB_TYPE_RELATION); }

static inline QString xCalMimeType() { return QString::fromLatin1(MIME_TYPE_XCAL); };
static inline QString xCardMimeType() { return QString::fromLatin1(MIME_TYPE_XCARD); };
static inline QString kolabMimeType() { return QString::fromLatin1(MIME_TYPE_KOLAB); };

KCalCore::Event::Ptr readV2EventXML(const QByteArray& xmlData, QStringList& attachments)
{
    return fromXML<KCalCore::Event::Ptr, KolabV2::Event>(xmlData, attachments);
}

QString ownUrlDecode(QByteArray encodedParam)
{
    encodedParam.replace('+', ' ');
    return QUrl::fromPercentEncoding(encodedParam);
}

RelationMember parseMemberUrl(const QString &string)
{
    if (string.startsWith("urn:uuid:")) {
        RelationMember member;
        member.gid = string.mid(9);
        return member;
    }
    QUrl url(QUrl::fromEncoded(string.toLatin1()));
    QList<QByteArray> path;
    Q_FOREACH(const QByteArray &fragment, url.encodedPath().split('/')) {
        path.append(ownUrlDecode(fragment).toUtf8());
    }
    // qDebug() << path;
    bool isShared = false;
    int start = path.indexOf("user");
    if (start < 0) {
        start = path.indexOf("shared");
        isShared = true;
    }
    if (start < 0) {
        Warning() << "Couldn't find \"user\" or \"shared\" in path: " << path;
        return RelationMember();
    }
    path = path.mid(start + 1);
    if (path.size() < 2) {
        Warning() << "Incomplete path: " << path;
        return RelationMember();
    }
    RelationMember member;
    if (!isShared) {
        member.user = path.takeFirst();
    }
    member.uid = path.takeLast().toLong();
    member.mailbox = path;
    member.messageId = ownUrlDecode(url.encodedQueryItemValue("message-id"));
    member.subject = ownUrlDecode(url.encodedQueryItemValue("subject"));
    member.date = ownUrlDecode(url.encodedQueryItemValue("date"));
    // qDebug() << member.uid << member.mailbox;
    return member;
}

static QByteArray join(const QList<QByteArray> &list, const QByteArray &c)
{
    QByteArray result;
    Q_FOREACH (const QByteArray &a, list) {
        result += a + c;
    }
    result.chop(c.size());
    return result;
}

KOLAB_EXPORT QString generateMemberUrl(const RelationMember &member)
{
    if (!member.gid.isEmpty()) {
        return QString("urn:uuid:%1").arg(member.gid);
    }
    QUrl url;
    url.setScheme("imap");
    QList<QByteArray> path;
    path << "/";
    if (!member.user.isEmpty()) {
        path << "user";
        path << QUrl::toPercentEncoding(member.user.toLatin1());
    } else {
        path << "shared";
    }
    Q_FOREACH(const QByteArray &mb, member.mailbox) {
        path << QUrl::toPercentEncoding(mb);
    }
    path << QByteArray::number(member.uid);
    url.setEncodedPath("/" + join(path, "/"));

    QList<QPair<QByteArray, QByteArray> > queryItems;
    queryItems.append(qMakePair(QString::fromLatin1("message-id").toLatin1(), QUrl::toPercentEncoding(member.messageId)));
    queryItems.append(qMakePair(QString::fromLatin1("subject").toLatin1(), QUrl::toPercentEncoding(member.subject)));
    queryItems.append(qMakePair(QString::fromLatin1("date").toLatin1(), QUrl::toPercentEncoding(member.date)));
    url.setEncodedQueryItems(queryItems);

    return QString::fromLatin1(url.toEncoded());
}

//@cond PRIVATE
class KolabObjectReader::Private
{
public:
    Private()
    :   mObjectType( InvalidObject ),
        mVersion( KolabV3 ),
        mOverrideObjectType(InvalidObject),
        mDoOverrideVersion(false)
    {
        mAddressee = KContacts::Addressee();
    }

    KCalCore::Incidence::Ptr mIncidence;
    KContacts::Addressee mAddressee;
    KContacts::ContactGroup mContactGroup;
    KMime::Message::Ptr mNote;
    QStringList mDictionary;
    QString mDictionaryLanguage;
    ObjectType mObjectType;
    Version mVersion;
    Kolab::Freebusy mFreebusy;
    ObjectType mOverrideObjectType;
    Version mOverrideVersion;
    bool mDoOverrideVersion;
    Akonadi::Relation mRelation;
    Akonadi::Tag mTag;
    QStringList mTagMembers;
};
//@endcond

KolabObjectReader::KolabObjectReader()
: d( new KolabObjectReader::Private )
{
}

KolabObjectReader::KolabObjectReader(const KMime::Message::Ptr& msg)
: d( new KolabObjectReader::Private )
{
    parseMimeMessage(msg);
}

KolabObjectReader::~KolabObjectReader()
{
    delete d;
}

void KolabObjectReader::setObjectType(ObjectType type)
{
    d->mOverrideObjectType = type;
}

void KolabObjectReader::setVersion(Version version)
{
    d->mOverrideVersion = version;
    d->mDoOverrideVersion = true;
}

void printMessageDebugInfo(const KMime::Message::Ptr &msg)
{
    //TODO replace by Debug stream for Mimemessage
    Debug() << "MessageId: " << msg->messageID()->asUnicodeString();
    Debug() << "Subject: " << msg->subject()->asUnicodeString();
//     Debug() << msg->encodedContent();
}

ObjectType KolabObjectReader::parseMimeMessage(const KMime::Message::Ptr &msg)
{
    ErrorHandler::clearErrors();
    d->mObjectType = InvalidObject;
    if (msg->contents().isEmpty()) {
        Critical() << "message has no contents (we likely failed to parse it correctly)";
        printMessageDebugInfo(msg);
        return InvalidObject;
    }
    const std::string message = msg->encodedContent().toStdString();
    Kolab::MIMEObject mimeObject;
    mimeObject.setObjectType(d->mOverrideObjectType);
    if (d->mDoOverrideVersion) {
        mimeObject.setVersion(d->mOverrideVersion);
    }
    d->mObjectType = mimeObject.parseMessage(message);
    d->mVersion = mimeObject.getVersion();
    switch (mimeObject.getType()) {
        case EventObject: {
            const Kolab::Event & event = mimeObject.getEvent();
            d->mIncidence = Kolab::Conversion::toKCalCore(event);
        }
            break;
        case TodoObject: {
            const Kolab::Todo & event = mimeObject.getTodo();
            d->mIncidence = Kolab::Conversion::toKCalCore(event);
        }
            break;
        case JournalObject: {
            const Kolab::Journal & event = mimeObject.getJournal();
            d->mIncidence = Kolab::Conversion::toKCalCore(event);
        }
            break;
        case ContactObject: {
            const Kolab::Contact &contact = mimeObject.getContact();
            d->mAddressee = Kolab::Conversion::toKABC(contact); //TODO extract attachments
        }
            break;
        case DistlistObject: {
            const Kolab::DistList &distlist = mimeObject.getDistlist();
            d->mContactGroup = Kolab::Conversion::toKABC(distlist);
        }
            break;
        case NoteObject: {
            const Kolab::Note &note = mimeObject.getNote();
            d->mNote = Kolab::Conversion::toNote(note);
        }
            break;
        case DictionaryConfigurationObject: {
            const Kolab::Configuration &configuration = mimeObject.getConfiguration();
            const Kolab::Dictionary &dictionary = configuration.dictionary();
            d->mDictionary.clear();
            foreach (const std::string &entry, dictionary.entries()) {
                d->mDictionary.append(Conversion::fromStdString(entry));
            }
            d->mDictionaryLanguage = Conversion::fromStdString(dictionary.language());
        }
            break;
        case FreebusyObject: {
            const Kolab::Freebusy &fb = mimeObject.getFreebusy();
            d->mFreebusy = fb;
            break;
        }
        case RelationConfigurationObject: {
            const Kolab::Configuration &configuration = mimeObject.getConfiguration();
            const Kolab::Relation &relation = configuration.relation();

            if (relation.type() == "tag") {
                d->mTag = Akonadi::Tag();
                d->mTag.setName(Conversion::fromStdString(relation.name()));
                d->mTag.setGid(Conversion::fromStdString(configuration.uid()).toLatin1());
                d->mTag.setType(Akonadi::Tag::GENERIC);

                d->mTagMembers.reserve(relation.members().size());
                foreach (const std::string &member, relation.members()) {
                    d->mTagMembers << Conversion::fromStdString(member);
                }
            } else if (relation.type() == "generic") {
                if (relation.members().size() == 2) {
                    d->mRelation = Akonadi::Relation();
                    d->mRelation.setRemoteId(Conversion::fromStdString(configuration.uid()).toLatin1());
                    d->mRelation.setType(Akonadi::Relation::GENERIC);

                    d->mTagMembers.reserve(relation.members().size());
                    foreach (const std::string &member, relation.members()) {
                        d->mTagMembers << Conversion::fromStdString(member);
                    }
                } else {
                    Critical() << "generic relation had wrong number of members:" << relation.members().size();
                    printMessageDebugInfo(msg);
                }
            } else {
                Critical() << "unknown configuration object type" << relation.type();
                printMessageDebugInfo(msg);
            }
        }
            break;
        default:
            Critical() << "no kolab object found ";
            printMessageDebugInfo(msg);
            break;
    }
    return d->mObjectType;
}

Version KolabObjectReader::getVersion() const
{
    return d->mVersion;
}

ObjectType KolabObjectReader::getType() const
{
    return d->mObjectType;
}

KCalCore::Event::Ptr KolabObjectReader::getEvent() const
{
    return d->mIncidence.dynamicCast<KCalCore::Event>();
}

KCalCore::Todo::Ptr KolabObjectReader::getTodo() const
{
    return d->mIncidence.dynamicCast<KCalCore::Todo>();
}

KCalCore::Journal::Ptr KolabObjectReader::getJournal() const
{
    return d->mIncidence.dynamicCast<KCalCore::Journal>();
}

KCalCore::Incidence::Ptr KolabObjectReader::getIncidence() const
{
    return d->mIncidence;
}

KContacts::Addressee KolabObjectReader::getContact() const
{
    return d->mAddressee;
}

KContacts::ContactGroup KolabObjectReader::getDistlist() const
{
    return d->mContactGroup;
}

KMime::Message::Ptr KolabObjectReader::getNote() const
{
    return d->mNote;
}

QStringList KolabObjectReader::getDictionary(QString& lang) const
{
    lang = d->mDictionaryLanguage;
    return d->mDictionary;
}

Freebusy KolabObjectReader::getFreebusy() const
{
    return d->mFreebusy;
}

bool KolabObjectReader::isTag() const
{
    return !d->mTag.gid().isEmpty();
}

Akonadi::Tag KolabObjectReader::getTag() const
{
    return d->mTag;
}

QStringList KolabObjectReader::getTagMembers() const
{
    return d->mTagMembers;
}

bool KolabObjectReader::isRelation() const
{
    return d->mRelation.isValid();
}

Akonadi::Relation KolabObjectReader::getRelation() const
{
    return d->mRelation;
}

static KMime::Message::Ptr createMimeMessage(const std::string &mimeMessage)
{
    KMime::Message::Ptr msg(new KMime::Message);
    msg->setContent(QByteArray(mimeMessage.c_str()));
    msg->parse();
    return msg;
}

KMime::Message::Ptr KolabObjectWriter::writeEvent(const KCalCore::Event::Ptr &i, Version v, const QString &productId, const QString &)
{
    ErrorHandler::clearErrors();
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    const Kolab::Event &event = Kolab::Conversion::fromKCalCore(*i);
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeEvent(event, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeTodo(const KCalCore::Todo::Ptr &i, Version v, const QString &productId, const QString &)
{
    ErrorHandler::clearErrors();
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    const Kolab::Todo &todo = Kolab::Conversion::fromKCalCore(*i);
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeTodo(todo, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeJournal(const KCalCore::Journal::Ptr &i, Version v, const QString &productId, const QString &)
{
    ErrorHandler::clearErrors();
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    const Kolab::Journal &journal = Kolab::Conversion::fromKCalCore(*i);
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeJournal(journal, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeIncidence(const KCalCore::Incidence::Ptr &i, Version v, const QString& productId, const QString& tz)
{
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    switch (i->type()) {
        case KCalCore::IncidenceBase::TypeEvent:
            return writeEvent(i.dynamicCast<KCalCore::Event>(),v,productId,tz);
        case KCalCore::IncidenceBase::TypeTodo:
            return writeTodo(i.dynamicCast<KCalCore::Todo>(),v,productId,tz);
        case KCalCore::IncidenceBase::TypeJournal:
            return writeJournal(i.dynamicCast<KCalCore::Journal>(),v,productId,tz);
        default:
            Critical() << "unknown incidence type";
    }
    return KMime::Message::Ptr();
}


KMime::Message::Ptr KolabObjectWriter::writeContact(const KContacts::Addressee &addressee, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    const Kolab::Contact &contact = Kolab::Conversion::fromKABC(addressee);
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeContact(contact, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeDistlist(const KContacts::ContactGroup &kDistList, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    const Kolab::DistList &distlist = Kolab::Conversion::fromKABC(kDistList);
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeDistlist(distlist, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeNote(const KMime::Message::Ptr &n, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    if (!n) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    const Kolab::Note &note = Kolab::Conversion::fromNote(n);
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeNote(note, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeDictionary(const QStringList &entries, const QString& lang, Version v, const QString& productId)
{
    ErrorHandler::clearErrors();

    Kolab::Dictionary dictionary(Conversion::toStdString(lang));
    std::vector <std::string> ent;
    foreach (const QString &e, entries) {
        ent.push_back(Conversion::toStdString(e));
    }
    dictionary.setEntries(ent);
    Kolab::Configuration configuration(dictionary); //TODO preserve creation/lastModified date
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeConfiguration(configuration, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeFreebusy(const Freebusy &freebusy, Version v, const QString& productId)
{
    ErrorHandler::clearErrors();
    Kolab::MIMEObject mimeObject;
    const std::string mimeMessage = mimeObject.writeFreebusy(freebusy, v, productId.toStdString());
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr writeRelationHelper(const Kolab::Relation &relation, const QByteArray &uid, const QString &productId)
{
    ErrorHandler::clearErrors();
    Kolab::MIMEObject mimeObject;

    Kolab::Configuration configuration(relation); //TODO preserve creation/lastModified date
    configuration.setUid(uid.constData());
    const std::string mimeMessage = mimeObject.writeConfiguration(configuration, Kolab::KolabV3, Conversion::toStdString(productId));
    return createMimeMessage(mimeMessage);
}

KMime::Message::Ptr KolabObjectWriter::writeTag(const Akonadi::Tag &tag, const QStringList &members, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    if (v != KolabV3) {
        Critical() << "only v3 implementation available";
    }

    Kolab::Relation relation(Conversion::toStdString(tag.name()), "tag");
    std::vector<std::string> m;
    m.reserve(members.count());
    foreach (const QString &member, members) {
        m.push_back(Conversion::toStdString(member));
    }
    relation.setMembers(m);

    return writeRelationHelper(relation, tag.gid(), productId);
}

KMime::Message::Ptr KolabObjectWriter::writeRelation(const Akonadi::Relation &relation, const QStringList &items, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    if (v != KolabV3) {
        Critical() << "only v3 implementation available";
    }

    if (items.size() != 2) {
        Critical() << "Wrong number of members for generic relation.";
        return KMime::Message::Ptr();
    }

    Kolab::Relation kolabRelation(std::string(), "generic");
    std::vector<std::string> m;
    m.reserve(2);
    m.push_back(Conversion::toStdString(items.at(0)));
    m.push_back(Conversion::toStdString(items.at(1)));
    kolabRelation.setMembers(m);

    return writeRelationHelper(kolabRelation, relation.remoteId(), productId);
}


}; //Namespace

