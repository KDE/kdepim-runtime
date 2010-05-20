/*
 *   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
 *   Copyright (c) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef KMINDEXREADER_H
#define KMINDEXREADER_H

#include "kmindexreader_export.h"

#include "messagestatus.h"
using KPIM::MessageStatus;

#include <QString>
#include <QStringList>
#include <QFile>
#include <QList>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

class KMIndexMsgPrivate;

/**
 * @short A class to read legacy kmail (< 4.5) index files
 *
 * This class provides read-only access to legacy kmail index files.
 * It uses old kmfolderindex code, authors attributed as appropriate.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class KMINDEXREADER_EXPORT KMIndexReader {
public:
    KMIndexReader ( const QString &indexFile );
   ~KMIndexReader();

    bool error() const;

    /**
     * begins the index reading process
     */
    bool readIndex();

    enum MsgPartType {
        MsgNoPart = 0,
        //unicode strings
        MsgFromPart = 1,
        MsgSubjectPart = 2,
        MsgToPart = 3,
        MsgReplyToIdMD5Part = 4,
        MsgIdMD5Part = 5,
        MsgXMarkPart = 6,
        //unsigned long
        MsgOffsetPart = 7,
        MsgLegacyStatusPart = 8,
        MsgSizePart = 9,
        MsgDatePart = 10,
        // unicode string
        MsgFilePart = 11,
        // unsigned long
        MsgCryptoStatePart = 12,
        MsgMDNSentPart = 13,
        //another two unicode strings
        MsgReplyToAuxIdMD5Part = 14,
        MsgStrippedSubjectMD5Part = 15,
        // and another unsigned long
        MsgStatusPart = 16,
        MsgSizeServerPart = 17,
        MsgUIDPart = 18,
        // unicode string
        MsgTagPart = 19
    };

    bool statusByOffset( quint64 offset, MessageStatus &status ) const;

    bool statusByFileName( const QString &fileName, MessageStatus &status ) const;

    bool imapUidByFileName( const QString &fileName, quint64 &uid ) const;

private:


    /**
     * Reads the header of an index
     */
    bool readHeader ( int *version );

    /**
     * creates a message object from an old index files
     */
    bool fromOldIndexString ( KMIndexMsgPrivate* msg, const QByteArray& str, bool toUtf8 );

    bool fillPartsCache ( KMIndexMsgPrivate* msg, off_t off, short int len );

    QList<KMIndexMsgPrivate*> messages();

    QString mIndexFileName;
    QFile mIndexFile;
    FILE* mFp;

    bool mConvertToUtf8;
    bool mIndexSwapByteOrder; // Index file was written with swapped byte order
    int mIndexSizeOfLong; // Index file was written with longs of this size
    off_t mHeaderOffset;

    bool mError;

    /** list of index entries or messages */
    QList<KMIndexMsgPrivate*> mMsgList;
    QHash<QString, KMIndexMsgPrivate*> mMsgByFileName;
    QHash<quint64, KMIndexMsgPrivate*> mMsgByOffset;
    friend class TestIdxReader;
};


class KMIndexMsgPrivate {
public:
    KMIndexMsgPrivate() {}
    /** Status object of the message. */
    MessageStatus& status();
    QStringList  tagList() const ;
    quint64 uid() const;

private:
    QString mCachedStringParts[20];
    unsigned long mCachedLongParts[20];
    bool mPartsCacheBuilt;

    MessageStatus mStatus;
    friend class KMIndexReader;
    friend class TestIdxReader;
};

#endif // KMINDEXREADER_H
