/*
    collectingprocess.cpp

    This file is part of libkdepim.
    Copyright (c) 2004 Ingo Kloecker <kloecker@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#include "collectingprocess.h"

#include <qvaluelist.h>

#include <string.h>

using namespace KPIM;

struct CollectingProcess::Private {
  Private() : stdoutSize( 0 ), stderrSize( 0 )
    {}

  uint stdoutSize;
  QValueList<QByteArray> stdoutBuffer;
  uint stderrSize;
  QValueList<QByteArray> stderrBuffer;
};


CollectingProcess::CollectingProcess( QObject * parent, const char * name )
  : KProcess( parent, name )
{
  d = new Private();
}

CollectingProcess::~CollectingProcess() {
  delete d; d = 0;
}

bool CollectingProcess::start( RunMode runmode, Communication comm ) {
  // prevent duplicate connection
  disconnect( this, SIGNAL( receivedStdout( KProcess *, char *, int ) ),
              this, SLOT( slotReceivedStdout( KProcess *, char *, int ) ) );
  if ( comm & Stdout ) {
    connect( this, SIGNAL( receivedStdout( KProcess *, char *, int ) ),
             this, SLOT( slotReceivedStdout( KProcess *, char *, int ) ) );
  }
  // prevent duplicate connection
  disconnect( this, SIGNAL( receivedStderr( KProcess *, char *, int ) ),
              this, SLOT( slotReceivedStderr( KProcess *, char *, int ) ) );
  if ( comm & Stderr ) {
    connect( this, SIGNAL( receivedStderr( KProcess *, char *, int ) ),
             this, SLOT( slotReceivedStderr( KProcess *, char *, int ) ) );
  }
  return KProcess::start( runmode, comm );
}

void CollectingProcess::slotReceivedStdout( KProcess *, char *buf, int len )
{
  QByteArray b;
  b.duplicate( buf, len );
  d->stdoutBuffer.append( b );
  d->stdoutSize += len;
}

void CollectingProcess::slotReceivedStderr( KProcess *, char *buf, int len )
{
  QByteArray b;
  b.duplicate( buf, len );
  d->stderrBuffer.append( b );
  d->stderrSize += len;
}

QByteArray CollectingProcess::collectedStdout()
{
  if ( d->stdoutSize == 0 ) {
    return QByteArray();
  }

  uint offset = 0;
  QByteArray b( d->stdoutSize );
  for ( QValueList<QByteArray>::const_iterator it = d->stdoutBuffer.begin();
        it != d->stdoutBuffer.end();
        ++it ) {
    memcpy( b.data() + offset, (*it).data(), (*it).size() );
    offset += (*it).size();
  }
  d->stdoutBuffer.clear();
  d->stdoutSize = 0;

  return b;
}

QByteArray CollectingProcess::collectedStderr()
{
  if ( d->stderrSize == 0 ) {
    return QByteArray();
  }

  uint offset = 0;
  QByteArray b( d->stderrSize );
  for ( QValueList<QByteArray>::const_iterator it = d->stderrBuffer.begin();
        it != d->stderrBuffer.end();
        ++it ) {
    memcpy( b.data() + offset, (*it).data(), (*it).size() );
    offset += (*it).size();
  }
  d->stderrBuffer.clear();
  d->stderrSize = 0;

  return b;
}

void CollectingProcess::virtual_hook( int id, void * data ) {
  KProcess::virtual_hook( id, data );
}

#include "collectingprocess.moc"
