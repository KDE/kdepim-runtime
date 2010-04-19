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

#ifndef KMINDEXREADER_H
#define KMINDEXREADER_H


#include <QString>
#include <QFile>

#include <unistd.h>
#include <stdio.h>

/**
 * @short A class to read legacy kmail (< 4.5) index files
 *
 * This class provides read-only access to legacy kmail index files. 
 * It uses old kmfolderindex code, authors attributed as appropriate.
 * @author Casey Link <unnamedrambler@gmail.com>
 */
class KMIndexReader
{
public:
  KMIndexReader( const QString &indexFile );
//   ~KMIndexReader() {}
  
  bool error() const;
  
//   bool readIndex();
  bool readHeader( int *version );
  
private:
  void clearIndex(bool autoDelete=true, bool syncDict = false);

  QString mIndexFileName;
  QFile mIndexFile;
  FILE* mFp;
  
  bool mConvertToUtf8;
  bool mIndexSwapByteOrder; // Index file was written with swapped byte order
  int mIndexSizeOfLong; // Index file was written with longs of this size
  off_t mHeaderOffset;

  bool mError;
  
    /** list of index entries or messages */
//   KMMsgList mMsgList;
  int mTotalMsgs;
};

#endif // KMINDEXREADER_H
