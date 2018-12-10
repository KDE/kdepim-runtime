/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KOLABXMLOBJECT_H
#define KOLABXMLOBJECT_H

#include "kolab_export.h"

#include <kolabformat.h>

#include "kolabdefinitions.h"

namespace Kolab {
class KOLAB_EXPORT XMLObject
{
public:
    XMLObject();

    std::string getSerializedUID() const;

    ///List of attachment names to be retrieved from the mime message (only when reading v2, for v3 attachments containing the cid: of the attachment-part are created )
    std::vector<std::string> getAttachments() const;

    Kolab::Event readEvent(const std::string &s, Kolab::Version version);
    std::string writeEvent(const Kolab::Event &, Kolab::Version version, const std::string &productId = std::string());

    Kolab::Todo readTodo(const std::string &s, Kolab::Version version);
    std::string writeTodo(const Kolab::Todo &, Kolab::Version version, const std::string &productId = std::string());

    Kolab::Journal readJournal(const std::string &s, Kolab::Version version);
    std::string writeJournal(const Kolab::Journal &, Kolab::Version version, const std::string &productId = std::string());

    Kolab::Freebusy readFreebusy(const std::string &s, Kolab::Version version);
    std::string writeFreebusy(const Kolab::Freebusy &, Kolab::Version version, const std::string &productId = std::string());

    std::string pictureAttachmentName() const;
    std::string logoAttachmentName() const;
    std::string soundAttachmentName() const;
    /**
     * Find the attachments and set them on the read Contact object.
     *
     * V2 Notes:
     * Picture, logo and sound must be retrieved from Mime Message attachments using they're corresponding attachment name.
     */
    Kolab::Contact readContact(const std::string &s, Kolab::Version version);

    /**
     * V2 Notes:
     * * Uses the following attachment names:
     * ** kolab-picture.png
     * ** kolab-logo.png
     * ** sound
     */
    std::string writeContact(const Kolab::Contact &, Kolab::Version version, const std::string &productId = std::string());

    Kolab::DistList readDistlist(const std::string &s, Kolab::Version version);
    std::string writeDistlist(const Kolab::DistList &, Kolab::Version version, const std::string &productId = std::string());

    /**
     * V2 notes:
     * * set the creation date from the mime date header.
     */
    Kolab::Note readNote(const std::string &s, Kolab::Version version);
    std::string writeNote(const Kolab::Note &, Kolab::Version version, const std::string &productId = std::string());

    Kolab::Configuration readConfiguration(const std::string &s, Kolab::Version version);
    std::string writeConfiguration(const Kolab::Configuration &, Kolab::Version version, const std::string &productId = std::string());

    Kolab::File readFile(const std::string &s, Kolab::Version version);
    std::string writeFile(const Kolab::File &, Kolab::Version version, const std::string &productId = std::string());

private:
    std::vector<std::string> mAttachments;
    std::string mLogoAttachmentName;
    std::string mSoundAttachmentName;
    std::string mPictureAttachmentName;
    std::string mWrittenUID;
};
}
#endif // KOLABXMLOBJECT_H
