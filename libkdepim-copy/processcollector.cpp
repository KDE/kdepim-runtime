/*  -*- mode: C++; c-file-style: "gnu" -*-

    This file is part of kdepim.
    Copyright (c) 2004 David Faure <faure@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
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

#include "processcollector.h"
#include <kprocess.h>

using namespace KPIM;

ProcessCollector::ProcessCollector( KProcess* process, QObject* parent, const char* name )
    : QObject( parent, name )
{
    connect( process, SIGNAL(receivedStdout(KProcess*,char*,int)),
             this, SLOT(slotCollectStdOut(KProcess*,char*,int)) );
    connect( process, SIGNAL(receivedStderr(KProcess*,char*,int)),
             this, SLOT(slotCollectStdErr(KProcess*,char*,int)) );
}

void ProcessCollector::slotCollectStdOut( KProcess *, char * buffer, int len )
{
  QByteArray & ba = mStdOutCollection;
  // append data to ba:
  int oldsize = ba.size();
  ba.resize( oldsize + len );
  qmemmove( ba.begin() + oldsize, buffer, len );
}

void ProcessCollector::slotCollectStdErr( KProcess *, char * buffer, int len )
{
  QByteArray & ba = mStdErrCollection;
  // append data to ba:
  int oldsize = ba.size();
  ba.resize( oldsize + len );
  qmemmove( ba.begin() + oldsize, buffer, len );
}

#include "processcollector.moc"
