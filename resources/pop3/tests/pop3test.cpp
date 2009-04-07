/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "pop3test.h"

#include <Akonadi/AgentManager>

#include <akonadi/qtest_akonadi.h>

QTEST_AKONADIMAIN( Pop3Test, NoGUI )

using namespace Akonadi;

void Pop3Test::initTestCase()
{
  Akonadi::AgentInstance::List agents = AgentManager::self()->instances();
  QCOMPARE( agents.size(), 2 );
  bool popFound = false;
  bool maildirFound = false;
  foreach( const AgentInstance &agent, agents ) {
    if ( agent.type().identifier() == "akonadi_maildir_resource" )
      maildirFound = true;
    if ( agent.type().identifier() == "akonadi_pop3_resource" )
      popFound = true;
  }

  QVERIFY( popFound );
  QVERIFY( maildirFound );
}

void Pop3Test::testSimpleDownload()
{
}

void Pop3Test::testSimpleLeaveOnServer()
{

}
