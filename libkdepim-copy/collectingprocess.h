/*  -*- mode: C++ -*-
    collectingprocess.h

    This file is part of libkdepim.
    Copyright (c) 2004 Ingo Kloecker

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

#ifndef __KPIM_COLLECTINGPROCESS_H__
#define __KPIM_COLLECTINGPROCESS_H__

#include <kprocess.h>
#include <kdepimmacros.h>

namespace KPIM {

/**
 * @short An output collecting KProcess class.
 *
 * This class simplifies the usage of KProcess by collecting all output
 * (stdout/stderr) of the process.
 *
 * @author Ingo Kloecker <kloecker@kde.org>
 */
class KDE_EXPORT CollectingProcess : public KProcess {
  Q_OBJECT
public:
  CollectingProcess( QObject * parent = 0, const char * name = 0 );
  ~CollectingProcess();

  /** Starts the process in NotifyOnExit mode and writes in to stdin of
      the process.
  */
  bool start( RunMode runmode, Communication comm );

  /** Returns the contents of the stdout buffer and clears it afterwards. */
  QByteArray collectedStdout();
  /** Returns the contents of the stderr buffer and clears it afterwards. */
  QByteArray collectedStderr();

private slots:
  void slotReceivedStdout( KProcess *, char *, int );
  void slotReceivedStderr( KProcess *, char *, int );

private:
  class Private;
  Private * d;
protected:
  void virtual_hook( int id, void * data );
};

} // namespace KPIM

#endif // __KPIM_COLLECTINGPROCESS_H__
