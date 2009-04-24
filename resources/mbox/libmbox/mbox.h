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

#include <QtCore/QSet>
#include <QtCore/QString>

#include "mbox_export.h"

typedef QPair<quint64, quint64> MsgInfo; // QPair<offset, size>

class MBOX_EXPORT MBox
{
  public:
    enum OpenMode {
      Normal,
      Reload
    };

    enum LockType {
      KDELockFile,           // Uses KLockFile
      ProcmailLockfile,
      MuttDotlock,
      MuttDotlockPrivileged,
      None
    };

  public:
    explicit MBox(const QString &mboxFile = QString(), bool readOnly = false);

    /**
     * Closes the file if it is still open.
     */
    ~MBox();

    /**
     * Closes the file and releases the lock.
     */
    void close();

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
    QList<MsgInfo> entryList(const QSet<int> &deletedItems = QSet<int>()) const;

    /**
     * Checks if the file exists and if it can be opened for read/write. Also
     * checks if the selected lock method is available when it is set to
     * procmail_lockfile or one of the mutt_dotlock variants.
     */
    bool isValid() const;

    /**
     * @see isValid()
     * @param errorMsg can be used to find out what kind of error occurred and
     *                 passed onto the user.
     */
    bool isValid(QString &errorMsg) const;

    /**
     * Open folder for access. Does nothing if the folder is already opened
     * and openMode equals Normal (default behavior). When Reload is given as
     * open mode the file will be closed first if it is open.
     *
     * Returns zero on success and an error code equal to the c-library fopen
     * call otherwise (errno).
     */
    int open(OpenMode openMode = Normal);


    /**
     * Reads the entire message from the file at given @param offset.
     */
    QByteArray readEntry(quint64 offset) const;

    /**
     * Reads the headers of the message at given @param offset.
     */
    QByteArray readEntryHeaders(quint64 offset);

    /**
     * Sets the locktype that should be used for locking the mbox file. The
     * isValid method will check if the lock method is available when the
     * procmail_lockfile or one of the mutt_dotlock variants is set.
     *
     * This method will not do anything if the mbox obeject is currently locked
     * to make sure that it doesn't leave a locked file for one of the lockfile
     * / mutt_dotlock methods.
     */
    void setLockType(LockType ltype);

    /**
     * Sets the lockfile that should be used by the procmail lock file method.
     * If this method is not called and procfile is used the name of the lock
     * file will be equal to MBOXFILENAME.lock.
     */
    void setProcmailLockFile(const QString &lockFile);

  private:
    /**
     * Locks the mbox file. Called by open(). Returns 0 on success and an errno
     * error code on failure.
     *
     * NOTE: This method will set the MBox object to ReadOnly mode when locking
     *       failed to prevent data corruption, even when the MBox was originally
     *       opened ReadWrite.
     */
    int lock();

    /**
     * Unlock the mbox file. Called by close() or ~MBox(). Returns 0 on success
     * and an errno error code on failure.
     */
    int unlock();

    /**
     * Unescapes the raw message read from the file.
     */
    static void unescapeFrom(char *msg, size_t size);

  private:
    class Private;
    Private *d;
};

#endif // MBOX_H
