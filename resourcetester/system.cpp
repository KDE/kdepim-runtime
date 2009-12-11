/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "system.h"
#include "test.h"
#include "global.h"

#include <KProcess>
#include <QFile>
#include <qtest_kde.h>

System::System(QObject* parent) :
  QObject( parent )
{
}

void System::exec(const QString &_program, const QStringList& args)
{
  QString program = _program;
  if ( QFile::exists( Global::basePath() + _program ) )
    program = Global::basePath() + _program;
  Test::instance()->verify( KProcess::execute( program, args ) == 0 );
}

void System::sleep(int secs)
{
  QTest::qWait( secs * 1000 );
}

#include "system.moc"
