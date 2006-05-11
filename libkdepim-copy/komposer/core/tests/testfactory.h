/*
 * testfactory.h
 *
 * Copyright (C)  2004  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#ifndef TESTFACTORY_H
#define TESTFACTORY_H

#include "managertest.h"

#include <Q3AsciiDict>

#define ADD_TEST(x) addTest( #x, new x )

class TestFactory
{
public:
    TestFactory()
        {
            ADD_TEST( ManagerTest );
            m_tests.setAutoDelete( true );
        }

    int runTests()
        {
            int result = 0;
            kDebug()<<"Running tests..."<<endl;
            QAsciiDictIterator<Tester> it( m_tests );
            for( ; it.current(); ++it ) {
                Tester* test = it.current();
                test->allTests();
                QStringList errorList = test->errorList();
                if ( !errorList.empty() ) {
                    ++result;
                    kDebug()<< it.currentKey() <<" errors:" << endl;
                    for ( QStringList::Iterator itr = errorList.begin();
                          itr != errorList.end(); ++itr ) {
                        kDebug()<< "\t" << (*itr).toLatin1() <<endl;;
                    }
                } else {
                    kDebug()<< it.currentKey()<< " OK "<<endl;
                }
            }
            kDebug()<< "Done" <<endl;
            return result;
        }
public:
    void addTest( const char* name, Tester* test )
        {
            m_tests.insert( name, test );
        }
private:
    QAsciiDict<Tester> m_tests;
};

#endif
