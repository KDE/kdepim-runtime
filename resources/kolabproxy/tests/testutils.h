
/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include <qsharedpointer.h>
#include <QSignalSpy>
#include <Akonadi/Monitor>
#include <Akonadi/Job>
#include <KCalCore/Event>

using namespace Akonadi;
#define TIMEOUT 1000
struct TestUtils
{
    static Akonadi::Item createImapItem(const KCalCore::Event::Ptr &event);

    typedef QPair<QSharedPointer<QSignalSpy>, QSharedPointer<Akonadi::Monitor> > MonitorPair;

    static MonitorPair monitor(Akonadi::Collection col, const char *signal);
    static bool wait(const MonitorPair &pair);
    static bool ensure(Akonadi::Collection col, const char *signal, Akonadi::Job *job);
    static bool ensurePopulated(QString agentinstance, int count);
    static Akonadi::Collection findCollection(QString agentinstance, QString name);
};
