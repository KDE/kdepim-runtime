#ifndef TESTER_H
#define TESTER_H

#include <kdebug.h>
#include <qstringlist.h>

#define CHECK( x, y ) check( __FILE__, __LINE__, #x, x, y )

class Tester
{
public:
    Tester() : m_tests( 0 ) {}
    virtual ~Tester() {}

public:
    virtual void allTests() = 0;

public:
    int testsFinished() const {
        return m_tests;
    }
    int testsFailed() const {
        return m_errorList.count();
    }
    QStringList errorList() const {
        return m_errorList;
    }

protected:
    template<typename T>
    void check( const char* file, int line, const char* str,
                const T& result, const T& expectedResult )
        {
            if ( result != expectedResult ) {
                QString error;
                QTextStream ts( &error, IO_WriteOnly );
                ts << file << "["<< line <<"]:"
                   <<" failed on \""<<  str <<"\""
                   << "\n\t\t result = "
                   << result
                   << ", expected = "<< expectedResult;
                m_errorList.append( error );
            }
            ++m_tests;
        }
private:
    QStringList m_errorList;
    int m_tests;
};

#endif
