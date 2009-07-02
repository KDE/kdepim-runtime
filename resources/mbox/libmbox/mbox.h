/*
    Copyright (c) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

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

#ifndef MBOX_H
#define MBOX_H

#include <boost/shared_ptr.hpp>
#include <kmime/kmime_message.h>
#include <QtCore/QSet>
#include <QtCore/QString>

#include "mbox_export.h"

typedef QPair<quint64, quint64> MsgInfo; // QPair<offset, size>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

class MBOX_EXPORT MBox
{
  public:
    enum LockType {
      ProcmailLockfile,
      MuttDotlock,
      MuttDotlockPrivileged,
      None
    };

  public:
    MBox();

    /**
     * Unlocks the file if it is still open.
     */
    ~MBox();

    /**
     * Appends @param entry to the MBox. Returns the offset in the file
     * where the added message starts or -1 if the entry was not added (e.g.
     * when it doesn't contain data). Entries are only added after a call to
     * load( const QString& ). The returned offset is <em>only</em> valid for
     * that particular file.
     *
     * @param entry The message to append to the mbox.
     * @return the offset of the entry in the file or -1 if the entry was not
     *         added.
     */
    qint64 appendEntry( const MessagePtr &entry );

    /**
     * Retrieve MsgInfo objects for all emails from the file except the
     * @param deleteItems. The @param deletedItems should be a list of file
     * offsets of messages which are deleted.
     *
     * Each MsgInfo object contains the offset and the size of the messages in
     * the file which are not marked as deleted.
     *
     * Note: One <em>must</em> call open() before calling this method.
     */
    QList<MsgInfo> entryList(const QSet<quint64> &deletedItems = QSet<quint64>()) const;

    /**
     * Loads a mbox on disk  into the current mbox. Messages already present are
     * *not* preserved. This method does not load the full messages into memory
     * but only the offsets of the messages and their sizes. If the file
     * currently is locked this method will do nothing and return false.
     * Appended messages that are not written yet will get lost.
     *
     * @param fileName the name of the mbox on disk.
     * @return true, if successful, false on error.
     *
     * @see save( const QString & )
     */
    bool load( const QString &fileName );

    /**
     * Locks the mbox file using the configured lock method. This can be used
     * for consecutive calls to readEntry and readEntryHeaders. Calling lock()
     * before these calls prevents the mbox file being locked for every call.
     *
     * @return true if locked successful, false on error.
     *
     * @see setLockType( LockType ), unlock()
     */
    bool lock();

    /**
     * Removes all messages at given offsets from the current reference file
     * (i.e. the file that is loaded with load( const QString & ) or the file
     * from the last save( const QString & ) call if that was not the same file).
     * This method will first check if all lines at the offsets are actually
     * seperator lines if this is not the no message will be deleted to prevent
     * corruption.
     *
     * @param deletedItems Offsets of the messages that should be removed from
     *                     the file.
     *
     * @return true if all offsets refer to a mbox seperator line and a file was
     *         loaded, false otherewhise. In the latter the physical file has
     *         not changed.
     */
    bool purge( const QSet<quint64> &deletedItems );

    /**
     * Reads the entire message from the file at given @param offset. If the
     * mbox file is not locked this method will lock the file before reading and
     * unlock it after reading. If the file already is locked, it will not
     * unlock the file after reading the entry.
     *
     * @param offset The start position of the entry in the mbox file.
     * @return Message at given offset or 0 if the the file could not be locked
     *         or the offset > fileSize.
     *
     * @see lock(), unlock()
     */
    KMime::Message *readEntry( quint64 offset );

    /**
     * Reads the headers of the message at given @param offset. If the
     * mbox file is not locked this method will lock the file before reading and
     * unlock it after reading. If the file already is locked, it will not
     * unlock the file after reading the entry.
     *
     * @param offset The start position of the entry in the mbox file.
     * @return QByteArray containing the raw Entry data.
     *
     * @see lock(), unlock()
     */
    QByteArray readEntryHeaders(quint64 offset);


    /**
     * Writes the mbox to disk. If the fileName is empty only appended messages
     * will be written to the file that was passed to load( const QString & ).
     * Otherwise the contents of the file that was loaded with load is copied to
     * @p fileName first.
     *
     * @param fileName the name of the file
     * @return true if the save was successful; false otherwise.
     *
     * @see load( const QString & )
     */
    bool save( const QString &fileName = QString() );

    /**
     * Sets the locktype that should be used for locking the mbox file. If the
     * new LockType cannot be used (e.g. the lockfile executable could not be
     * found) the LockType will not be changed.
     *
     * This method will not do anything if the mbox obeject is currently locked
     * to make sure that it doesn't leave a locked file for one of the lockfile
     * / mutt_dotlock methods.
     */
    bool setLockType(LockType ltype);

    /**
     * Sets the lockfile that should be used by the procmail or the KDE lock
     * file method. If this method is not called and one of the before mentioned
     * lock methods is used the name of the lock file will be equal to
     * MBOXFILENAME.lock.
     */
    void setLockFile(const QString &lockFile);

    /**
     * Unlock the mbox file.
     *
     * @return true if the unlock was successful, false otherwise.
     *
     * @see lock()
     */
    bool unlock();

  private:
    bool open();

    static QByteArray escapeFrom(const QByteArray &msg);

    /**
     * Generates a mbox message sperator line for given message.
     */
    static QByteArray mboxMessageSeparator(const QByteArray &msg);

    /**
     * Unescapes the raw message read from the file.
     */
    static void unescapeFrom(char *msg, size_t size);

  private:
    class Private;
    Private *d;
};

#endif // MBOX_H
