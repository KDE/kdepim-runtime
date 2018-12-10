/*
 * Copyright (C) 2012  Sofia Balicka <balicka@kolabsys.com>
 * Copyright (C) 2014  Christian Mollekopf <mollekopf@kolabsys.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MIMEOBJECT_H
#define MIMEOBJECT_H

#include "kolab_export.h"

#include <kolabformat.h>
#include "kolabdefinitions.h"

namespace Kolab {
class KOLAB_EXPORT MIMEObject
{
public:
    MIMEObject();
    ~MIMEObject();

    ObjectType parseMessage(const std::string &msg);

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

    Kolab::Event getEvent() const;
    Kolab::Todo getTodo() const;
    Kolab::Journal getJournal() const;
    Kolab::Note getNote() const;
    Kolab::Contact getContact() const;
    Kolab::DistList getDistlist() const;
    Kolab::Freebusy getFreebusy() const;
    Kolab::Configuration getConfiguration() const;

    std::string writeEvent(const Kolab::Event &event, Version version, const std::string &productId = std::string());
    Kolab::Event readEvent(const std::string &s);

    std::string writeTodo(const Kolab::Todo &todo, Version version, const std::string &productId = std::string());
    Kolab::Todo readTodo(const std::string &s);

    std::string writeJournal(const Kolab::Journal &journal, Version version, const std::string &productId = std::string());
    Kolab::Journal readJournal(const std::string &s);

    std::string writeNote(const Kolab::Note &note, Version version, const std::string &productId = std::string());
    Kolab::Note readNote(const std::string &s);

    std::string writeContact(const Kolab::Contact &contact, Version version, const std::string &productId = std::string());
    Kolab::Contact readContact(const std::string &s);

    std::string writeDistlist(const Kolab::DistList &distlist, Version version, const std::string &productId = std::string());
    Kolab::DistList readDistlist(const std::string &s);

    std::string writeFreebusy(const Kolab::Freebusy &freebusy, Version version, const std::string &productId = std::string());
    Kolab::Freebusy readFreebusy(const std::string &s);

    std::string writeConfiguration(const Kolab::Configuration &freebusy, Version version, const std::string &productId = std::string());
    Kolab::Configuration readConfiguration(const std::string &s);

private:
    //@cond PRIVATE
    MIMEObject(const MIMEObject &other);
    MIMEObject &operator=(const MIMEObject &rhs);
    class Private;
    Private *const d;
    //@endcond
};
}
#endif
