/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef HANDLERTEST_H
#define HANDLERTEST_H

#include <QtTest/QtTest>

#include "handler.h"
#include "response.h"

using namespace Akonadi;

class HandlerTest: public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void testInit();
    void testSeparatorList();
    void testRootPercentList();
    void testRootStarList();
    void testInboxList();
    void testFetch();
    
  private:
    // Helper
    Response nextResponse( QSignalSpy& spy );
    Handler* getHandlerFor( const QByteArray& command );
};

#endif
