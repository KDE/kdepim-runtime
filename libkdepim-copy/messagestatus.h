/*  -*- mode: C++ -*-
    This file is part of KDEPIM.
    Copyright (c) 2005 Andreas Gungl <a.gungl@gmx.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/
#ifndef KMAIL_MESSAGESTATUS_H
#define KMAIL_MESSAGESTATUS_H

#include <qstring.h>

class QString;

/** The message status format. These can be or'd together.
    Note, that the KMMsgStatusIgnored implies the
    status to be Read even if the flags are set
    to Unread or New. This is done in isRead()
    and related getters. So we can preserve the state
    when switching a thread to Ignored and back. */
enum MsgStatus
{
    KMMsgStatusUnknown =           0x00000000,
    KMMsgStatusNew =               0x00000001,
    KMMsgStatusUnread =            0x00000002,
    KMMsgStatusRead =              0x00000004,
    KMMsgStatusOld =               0x00000008,
    KMMsgStatusDeleted =           0x00000010,
    KMMsgStatusReplied =           0x00000020,
    KMMsgStatusForwarded =         0x00000040,
    KMMsgStatusQueued =            0x00000080,
    KMMsgStatusSent =              0x00000100,
    KMMsgStatusFlag =              0x00000200, // flag means important
    KMMsgStatusWatched =           0x00000400,
    KMMsgStatusIgnored =           0x00000800, // forces isRead()
    KMMsgStatusTodo =              0x00001000,
    KMMsgStatusSpam =              0x00002000,
    KMMsgStatusHam =               0x00004000,
    KMMsgStatusHasAttach =         0x00008000,
    KMMsgStatusHasNoAttach =       0x00010000  // to be removed before KDE 4
};

// TODO should become QInt32
typedef uint KMMsgStatus;


namespace KPIM {

//---------------------------------------------------------------------------
/**
  @short KDEPIM Message Status.
  @author Andreas Gungl <a.gungl@gmx.de>

  The class encapsulates the handling of the different flags
  which describe the status of a message.
  The flags itself are not intended to be used outside this class.

  In the status pairs Watched/Ignored and Spam/Ham, there both
  values can't be set at the same time, however they can
  be unset at the same time.

  The stati New/Unread/Read/Old are mutually exclusive.
*/
class MessageStatus
{
  public:
    /** Constructor - sets status initially to unknown. */
    MessageStatus();

    /** Assign the status from another instance. */
    MessageStatus& operator = ( const MessageStatus& other );

    /** Compare the status with that from another instance.
        @return TRUE if the stati are equal, false if different.
    */
    bool operator == ( const MessageStatus& other ) const;

    /** Compare the status with that from another instance.
        @return TRUE if the stati are equal, false if different.
    */
    bool operator != ( const MessageStatus& other ) const;

    /** Check, if some of the flags in the status match 
        with those flags from another instance.
        @return TRUE if at least one flag is set in both stati.
    */
    bool operator & ( const MessageStatus& other ) const;

    /** Clear all status flags, this resets to unknown. */
    void clear();

    /* ----- getters ----------------------------------------------------- */

    /** Check for Unknown status.
        @return TRUE if status is unknown.
    */
    bool isOfUnknownStatus() const;

    /** Check for New status. Ignored messages are not new.
        @return TRUE if status is new.
    */
    bool isNew() const;

    /** Check for Unread status. Note that new messages are not unread.
        Ignored messages are not unread as well.
        @return TRUE if status is unread.
    */
    bool isUnread() const;

    /** Check for Read status. Note that ignored messages are read.
        @return TRUE if status is read.
    */
    bool isRead() const;

    /** Check for Old status.
        @return TRUE if status is old.
    */
    bool isOld() const;

    /** Check for Deleted status.
        @return TRUE if status is deleted.
    */
    bool isDeleted() const;

    /** Check for Replied status.
        @return TRUE if status is replied.
    */
    bool isReplied() const;

    /** Check for Forwarded status.
        @return TRUE if status is forwarded.
    */
    bool isForwarded() const;

    /** Check for Queued status.
        @return TRUE if status is queued.
    */
    bool isQueued() const;

    /** Check for Sent status.
        @return TRUE if status is sent.
    */
    bool isSent() const;

    /** Check for Important status.
        @return TRUE if status is important.
    */
    bool isImportant() const;

    /** Check for Watched status.
        @return TRUE if status is watched.
    */
    bool isWatched() const;

    /** Check for Ignored status.
        @return TRUE if status is ignored.
    */
    bool isIgnored() const;

    /** Check for Todo status.
        @return TRUE if status is todo.
    */
    bool isTodo() const;

    /** Check for Spam status.
        @return TRUE if status is spam.
    */
    bool isSpam() const;

    /** Check for Ham status.
        @return TRUE if status is not spam.
    */
    bool isHam() const;

    /** Check for Attachment status.
        @return TRUE if status indicates an attachment.
    */
    bool hasAttachment() const;

    /* ----- setters ----------------------------------------------------- */

    /** Set the status to new. */
    void setNew();

    /** Set the status to unread. */
    void setUnread();

    /** Set the status to read. */
    void setRead();

    /** Set the status to old. */
    void setOld();

    /** Set the status for deleted.
        @param deleted Set (true) or unset (false) this status flag.
    */
    void setDeleted( bool deleted = true );

    /** Set the status for replied.
        @param replied Set (true) or unset (false) this status flag.
    */
    void setReplied( bool replied = true );

    /** Set the status for forwarded.
        @param forwarded Set (true) or unset (false) this status flag.
    */
    void setForwarded( bool forwarded = true );

    /** Set the status for queued.
        @param queued Set (true) or unset (false) this status flag.
    */
    void setQueued( bool queued = true );

    /** Set the status for sent.
        @param sent Set (true) or unset (false) this status flag.
    */
    void setSent(  bool sent = true );

    /** Set the status for important.
        @param important Set (true) or unset (false) this status flag.
    */
    void setImportant( bool important = true );

    /** Set the status to watched.
        @param watched Set (true) or unset (false) this status flag.
    */
    void setWatched( bool watched = true );

    /** Set the status to ignored.
        @param ignored Set (true) or unset (false) this status flag.
    */
    void setIgnored( bool ignored = true );

    /** Set the status to todo.
        @param ignored Set (true) or unset (false) this status flag.
    */
    void setTodo( bool todo = true );

    /** Set the status to spam.
        @param spam Set (true) or unset (false) this status flag.
    */
    void setSpam( bool spam = true );

    /** Set the status to not spam.
        @param ham Set (true) or unset (false) this status flag.
    */
    void setHam( bool ham = true );

    /** Set the status for an attachment.
        @param deleted Set (true) or unset (false) this status flag.
    */
    void setHasAttachment( bool withAttachment = true );

    /* ----- state representation  --------------------------------------- */

    /** Get the status as a whole e.g. for storage in an index.
        Don't manipulte the index via this value, this bypasses
        all integrity checks in the setter methods.
        @return The status encoded in bits.
    */
    const KMMsgStatus toQInt32() const;

    /** Set the status as a whole e.g. for reading from an index.
        Don't manipulte the index via this value, this bypasses
        all integrity checks in the setter methods.
        @param status The status encoded in bits to be set in this instance.
    */
    void fromQInt32( KMMsgStatus status );

    /** Convert the status to a string representation.
        @return A string containing coded upercase letters
                which describe the status.
    */
    QString getStatusStr() const;

    /** Set the status based on a string representation.
        @param aStr The status string to be analyzed.
                    Normally its a string obtained using
                    getStatusStr().
    */
    void setStatusFromStr( QString aStr );

    /** Convert the status to a string for sorting.
        @return A string containing coded lowercase letters
                which allows a predefined sorting by status.
    */
    QString getSortRank() const;

    /* ----- static accessors to simple states --------------------------- */

    /** Return a predefined status initialized as New as is usefull
        e.g. when providing a state for comparison.
        @return A reference to a status instance initialized as New.
    */
    static MessageStatus statusNew();

    /** Return a predefined status initialized as Read as is usefull
        e.g. when providing a state for comparison.
        @return A reference to a status instance initialized as Read.
    */
    static MessageStatus statusRead();

    /** Return a predefined status initialized as Unread as is usefull
        e.g. when providing a state for comparison.
        @return A reference to a status instance initialized as Unread.
    */
    static MessageStatus statusUnread();

  private:
    KMMsgStatus mStatus;
};

} // namespace KPIM

#endif /*KMAIL_MESSAGESTATUS_H*/
