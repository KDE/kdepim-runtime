/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "kolab_export.h"

#include <Akonadi/Item>
#include <Akonadi/Tag>
#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>
#include <KContacts/Addressee>
#include <KContacts/ContactGroup>
#include <KMime/Message>

#include "kolabdefinitions.h"

#include <memory>

namespace Kolab
{
class Freebusy;

KOLAB_EXPORT KCalendarCore::Event::Ptr readV2EventXML(const QByteArray &xmlData, QStringList &attachments);

struct KOLAB_EXPORT RelationMember {
    QString messageId;
    QString subject;
    QString date;
    QList<QByteArray> mailbox;
    QString user;
    qint64 uid;
    QString gid;
};
KOLAB_EXPORT RelationMember parseMemberUrl(const QString &url);
KOLAB_EXPORT QString generateMemberUrl(const RelationMember &url);

class KolabObjectReaderPrivate;

/**
 * Class to read Kolab Mime files
 *
 * It implements the Kolab specifics of Mime message handling.
 * This class is not reusable and only meant to read a single object.
 * Parse the mime message and then call the correct getter, based on the type
 *
 */
class KOLAB_EXPORT KolabObjectReader
{
public:
    KolabObjectReader();
    explicit KolabObjectReader(const std::shared_ptr<KMime::Message> &msg);
    ~KolabObjectReader();

    ObjectType parseMimeMessage(const std::shared_ptr<KMime::Message> &msg);

    /**
     * Set to override the autodetected object type, before parsing the message.
     */
    void setObjectType(ObjectType);

    /**
     * Set to override the autodetected version, before parsing the message.
     */
    void setVersion(Version);

    /**
     * Returns the Object type of the parsed kolab object.
     */
    ObjectType getType() const;
    /**
     * Returns the kolab-format version of the parsed kolab object.
     */
    Version getVersion() const;

    /**
     * Getter to get the retrieved object.
     * Only the correct one will return a valid object.
     *
     * Use getType() to determine the correct one to call.
     */
    KCalendarCore::Event::Ptr getEvent() const;
    KCalendarCore::Todo::Ptr getTodo() const;
    KCalendarCore::Journal::Ptr getJournal() const;
    KCalendarCore::Incidence::Ptr getIncidence() const;
    KContacts::Addressee getContact() const;
    KContacts::ContactGroup getDistlist() const;
    QStringList getDictionary(QString &lang) const;
    Freebusy getFreebusy() const;
    bool isTag() const;
    Akonadi::Tag getTag() const;
    QStringList getTagMembers() const;

private:
    //@cond PRIVATE
    KolabObjectReader(const KolabObjectReader &other) = delete;
    KolabObjectReader &operator=(const KolabObjectReader &rhs) = delete;

private:
    std::unique_ptr<KolabObjectReaderPrivate> const d;
    //@endcond
};

/**
 * Class to write Kolab Mime files
 *
 */
class KOLAB_EXPORT KolabObjectWriter
{
public:
    static std::shared_ptr<KMime::Message>
    writeEvent(const KCalendarCore::Event::Ptr &, Version v = KolabV3, const QString &productId = QString(), const QString &tz = QString());
    static std::shared_ptr<KMime::Message>
    writeTodo(const KCalendarCore::Todo::Ptr &, Version v = KolabV3, const QString &productId = QString(), const QString &tz = QString());
    static std::shared_ptr<KMime::Message>
    writeJournal(const KCalendarCore::Journal::Ptr &, Version v = KolabV3, const QString &productId = QString(), const QString &tz = QString());
    static std::shared_ptr<KMime::Message>
    writeIncidence(const KCalendarCore::Incidence::Ptr &, Version v = KolabV3, const QString &productId = QString(), const QString &tz = QString());
    static std::shared_ptr<KMime::Message> writeContact(const KContacts::Addressee &, Version v = KolabV3, const QString &productId = QString());
    static std::shared_ptr<KMime::Message> writeDistlist(const KContacts::ContactGroup &, Version v = KolabV3, const QString &productId = QString());
    static std::shared_ptr<KMime::Message> writeDictionary(const QStringList &, const QString &lang, Version v = KolabV3, const QString &productId = QString());
    static std::shared_ptr<KMime::Message> writeFreebusy(const Kolab::Freebusy &, Version v = KolabV3, const QString &productId = QString());
    static std::shared_ptr<KMime::Message> writeTag(const Akonadi::Tag &, const QStringList &items, Version v = KolabV3, const QString &productId = QString());
};
} // Namespace
