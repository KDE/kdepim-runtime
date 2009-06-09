/*
    Copyright (c) 2009 Thomas McGuire <mcguire@kde.org>

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
#include "kresmigrationtest.h"

#include <Akonadi/Control>
#include <Akonadi/AgentManager>
#include <KResources/Manager>
#include <KCal/ResourceCalendar>

#include <qtest_kde.h>

using namespace Akonadi;

QTEST_KDEMAIN( KResMigrationTester, NoGUI )

void KResMigrationTester::test_initalSetup()
{
  KRES::Manager<KCal::ResourceCalendar> calendarManager( "calendar" );
  calendarManager.readConfig();

  QList<KCal::ResourceCalendar*> resources;
  KRES::Manager< KCal::ResourceCalendar >::Iterator it = calendarManager.begin();
  while( it != calendarManager.end() ) {
    resources.append( *it );
    it++;
  }
  QCOMPARE( resources.size(), 1 );
  KCal::ResourceCalendar *akonadi = resources.front();
  QCOMPARE( akonadi->resourceName(), QLatin1String( "Akonadi Compatibility Resource" ) );
  QCOMPARE( akonadi->type(), QLatin1String( "akonadi" ) );
  QVERIFY( calendarManager.standardResource() == akonadi );

  // The bridge should have an actual subresource that will hold the data
  QCOMPARE( akonadi->subresources().size(), 1 );
  // TODO: More sophisticated tests, also test the name and type of the subresource
  // TODO: Check that the actual akonadi resource for this exists

  // Try creating one event and adding it to the resource. There should not be
  // a GUI popup to select some folder and no error message, everything should work
  // out of the box.
  // Since the test is a NoGui test it crashes/fails when that happen.
  // TODO: is there a better way to check that some standard subresource is set up?
  KCal::Incidence *i = new KCal::Journal();
  i->setSummary( "Hello World" );
  akonadi->addIncidence(i);
  akonadi->save();

  // TODO: repeat same checks when Akonadi is not started
  // -> control::stop
  // -> remove akonadi resources
  // -> reset migrator state (how to do that?)
  // TODO: Same checks for kabc
}
