/**
 * tester.h
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 */
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
                   << "\ntests:\t\t result = "
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
