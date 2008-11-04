/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#ifndef __MAILDIR_H__
#define __MAILDIR_H__


#include <maildir_export.h>

#include <QString>
#include <QStringList>

namespace KPIM {

class MAILDIR_EXPORT Maildir
{
public:
    explicit Maildir( const QString& path = QString(), bool isRoot = false );
    /* Copy constructor */
    Maildir(const Maildir & rhs);
    /* Copy operator */
    Maildir& operator=(const Maildir & rhs);
    /** Equality comparison */
    bool operator==(const Maildir & rhs) const;
    /* Destructor */
    ~Maildir();

    /** Returns whether the maildir has all the necessary subdirectories,
     * that they are readable, etc.  */
    bool isValid() const;

    /**
     * Returns whether the maildir is valid, and sets the error out-parameter
     * so it can be used to signal the kind of error to the user.
     * @see isValid
     */
    bool isValid( QString &error ) const;

    /**
     * Make a valid maildir at the path of this Maildir object. This involves
     * creating the necessary subdirs, etc. Note that an empty Maildir is
     * not valid, unless it is given  valid path, or until create( ) is
     * called on it.
     */
    bool create();

    /**
     * Returns the path of this maildir.
     */
    QString path() const;

    /**
     * Returns the list of items (mails) in the maildir. These are keys, which
     * map to filenames, internally, but that's an implementation detail, which
     * should not be relied on.
     */
    QStringList entryList() const;

    /**
     * Returns the list of subfolders, as names (relative paths). Use the
     * subFolder method to get Maildir objects representing them.
     */
    QStringList subFolderList() const;

    /**
     * Adds subfolder with the given @param folderName.
     * @return an empty string on failure or the full path of the new subfolder
     *         on success
     */
    QString addSubFolder( const QString& folderName );

    /**
     * Removes subfolder with the given @param folderName. Returns success or failure.
     */
    bool removeSubFolder( const QString& folderName );

    /**
     * Returns a Maildir object for the given @param folderName. If such a folder
     * exists, the Maildir object will be valid, otherwise you can call create()
     * on it, to make a subfolder with that name.
     */
    Maildir subFolder( const QString& folderName );

    /**
     * Returns the size of the file in the maildir with the given @param key.
     * @since 4.2
     */
    qint64 size( const QString& key ) const;

    /**
     * Return the contents of the file in the maildir with the given @param key.
     */
    QByteArray readEntry( const QString& key ) const;

    /**
     * Return the contents of the headers section of the file the maildir with the given @param key.
     */
    QByteArray readEntryHeaders( const QString& key ) const;

    /**
     * Write the given @param data to a file in the maildir with the given
     * @param key.
     */
    void writeEntry( const QString& key, const QByteArray& data );

    /**
     * Adds the given @param to the maildir. Returns the key of the entry.
     */
    QString addEntry( const QByteArray& data );

    /**
     * Removes the entry with the given @key. Returns success or failure.
     */
    bool removeEntry( const QString& key );
private:
    void swap( const Maildir& );
    class Private;
    Private *d;
};

}
#endif // __MAILDIR_H__
