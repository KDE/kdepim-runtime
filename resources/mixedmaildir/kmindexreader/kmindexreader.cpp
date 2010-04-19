/*
 *   Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
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

#include "kmindexreader.h"


#include <KDebug>
#include <kde_file.h>

#include <QFile>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

#define INDEX_VERSION 1506
#ifndef MAX_LINE
#define MAX_LINE 4096
#endif

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

KMIndexReader::KMIndexReader(const QString& indexFile) 
: mIndexFileName( indexFile )
, mIndexFile( indexFile )
, mIndexSwapByteOrder( false )
, mHeaderOffset( 0 )
, mError( false )
{
  if( !mIndexFile.exists() )
  {
    kDebug() << "file doesn't exist";
    mError = true;
  }

  if( !mIndexFile.open( QIODevice::ReadOnly ) )
  {
    kDebug() << "file cant be open";
    mError = true;
  }
  
  mFp = fdopen(mIndexFile.handle(), "r");
}

bool KMIndexReader::error() const
{
  return mError;
}

bool KMIndexReader::readHeader( int *version )
{
  int indexVersion;
  Q_ASSERT(mFp != 0);
  mIndexSwapByteOrder = false;
  mIndexSizeOfLong = sizeof(long);

  int ret = fscanf(mFp, "# KMail-Index V%d\n", &indexVersion);
  if ( ret == EOF || ret == 0 )
      return false; // index file has invalid header
  if(version)
      *version = indexVersion;
  if (indexVersion < 1505 ) {
      if(indexVersion == 1503) {
        kDebug() << "Converting old index file" << mIndexFileName << "to utf-8";
        mConvertToUtf8 = true;
      }
      return true;
  } else if (indexVersion == 1505) {
  } else if (indexVersion < INDEX_VERSION) {
      kDebug() << "Index file" << mIndexFileName << "is out of date. Re-creating it.";
//       createIndexFromContents();
      return false;
  } else if(indexVersion > INDEX_VERSION) {
      kDebug() << "index file of newer version";
      return false;
  }
  else {
      // Header
      quint32 byteOrder = 0;
      quint32 sizeOfLong = sizeof(long); // default

      quint32 header_length = 0;
      KDE_fseek(mFp, sizeof(char), SEEK_CUR );
      fread(&header_length, sizeof(header_length), 1, mFp);
      if (header_length > 0xFFFF)
         header_length = kmail_swap_32(header_length);

      off_t endOfHeader = KDE_ftell(mFp) + header_length;

      bool needs_update = true;
      // Process available header parts
      if (header_length >= sizeof(byteOrder))
      {
         fread(&byteOrder, sizeof(byteOrder), 1, mFp);
         mIndexSwapByteOrder = (byteOrder == 0x78563412);
         header_length -= sizeof(byteOrder);

         if (header_length >= sizeof(sizeOfLong))
         {
            fread(&sizeOfLong, sizeof(sizeOfLong), 1, mFp);
            if (mIndexSwapByteOrder)
               sizeOfLong = kmail_swap_32(sizeOfLong);
            mIndexSizeOfLong = sizeOfLong;
            header_length -= sizeof(sizeOfLong);
            needs_update = false;
         }
      }
      if (needs_update || mIndexSwapByteOrder || (mIndexSizeOfLong != sizeof(long)))
      {
	kDebug() << "DIRTY!";
//         setDirty( true );
      }
      // Seek to end of header
      KDE_fseek(mFp, endOfHeader, SEEK_SET );

      if ( mIndexSwapByteOrder ) {
         kDebug() << "Index File has byte order swapped!";
      }
      if ( mIndexSizeOfLong != sizeof( long ) ) {
         kDebug() << "Index File sizeOfLong is" << mIndexSizeOfLong << "while sizeof(long) is" << sizeof(long) << "!";
      }

  }
  return true;
}

// bool KMIndexReader::readIndex()
// {
//   qint32 len;
//   KMMsgInfo* mi;
// 
//   Q_ASSERT( mFp != 0 );
//   rewind(mFp);
// 
//   clearIndex();
//   int version;
// 
// //   setDirty( false );
// 
//   if (!readHeader(&version)) return false;
// 
// //   mUnreadMsgs = 0;
//   mTotalMsgs = 0;
//   mHeaderOffset = KDE_ftell(mFp);
// 
//   clearIndex();
//   while (!feof(mFp))
//   {
//     mi = 0;
//     if(version >= 1505) {
//       if(!fread(&len, sizeof(len), 1, mFp))
//         break;
// 
//       if (mIndexSwapByteOrder)
//         len = kmail_swap_32(len);
// 
//       off_t offs = KDE_ftell(mFp);
//       if(KDE_fseek(mFp, len, SEEK_CUR))
//         break;
//       mi = new KMMsgInfo(folder(), offs, len);
//     }
//     else
//     {
//       QByteArray line( MAX_LINE, '\0' );
//       fgets(line.data(), MAX_LINE, mFp);
//       if (feof(mFp)) break;
//       if (*line.data() == '\0') {
//         fclose(mFp);
//         //kDebug( KMKernel::storageDebug() ) << "fclose(mFp = " << mFp << ")";
//         mFp = 0;
//         clearIndex();
//         return false;
//       }
//       mi = new KMMsgInfo(folder());
//       mi->compat_fromOldIndexString(line, mConvertToUtf8);
//     }
//     if(!mi)
//       break;
// 
//     if (mi->status().isDeleted())
//     {
//       delete mi;  // skip messages that are marked as deleted
// //       setDirty( true );
//       needsCompact = true;  //We have deleted messages - needs to be compacted
//       continue;
//     }
// #ifdef OBSOLETE
//     else if (mi->isNew())
//     {
//       mi->setStatus(KMMsgStatusUnread);
//       mi->setDirty(false);
//     }
// #endif
//     if ((mi->status().isNew()) || (mi->status().isUnread()) ||
//         (folder() == kmkernel->outboxFolder()))
//     {
//       ++mUnreadMsgs;
//       if (mUnreadMsgs == 0) ++mUnreadMsgs;
//     }
//     mMsgList.append(mi, false);
//   }
//   if( version < 1505)
//   {
//     mConvertToUtf8 = false;
//     setDirty( true );
//     writeIndex();
//   }
//   mTotalMsgs = mMsgList.count();
//   return true;
// }
// 
// void KMIndexReader::clearIndex(bool autoDelete, bool syncDict)
// {
//   mMsgList.clear(autoDelete, syncDict);
// }



