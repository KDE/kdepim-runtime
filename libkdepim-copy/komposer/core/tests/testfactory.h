#ifndef TESTFACTORY_H
#define TESTFACTORY_H

#include "managertest.h"

#include <qasciidict.h>

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
            kdDebug()<<"Running tests..."<<endl;
            QAsciiDictIterator<Tester> it( m_tests );
            for( ; it.current(); ++it ) {
                Tester* test = it.current();
                test->allTests();
                QStringList errorList = test->errorList();
                if ( !errorList.empty() ) {
                    ++result;
                    kdDebug()<< it.currentKey() <<" errors:" << endl;
                    for ( QStringList::Iterator itr = errorList.begin();
                          itr != errorList.end(); ++itr ) {
                        kdDebug()<< "\t" << (*itr).latin1() <<endl;;
                    }
                } else {
                    kdDebug()<< it.currentKey()<< " OK "<<endl;
                }
            }
            kdDebug()<< "Done" <<endl;
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
