/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#ifndef __MAILDIR_H__
#define __MAILDIR_H__


#include <kdemacros.h>

#include <QString>
#include <QStringList>

namespace KPIM {

class KDE_EXPORT Maildir
{
public:
  Maildir( const QString& path = QString() );
  /* Copy constructor */
  Maildir(const Maildir & rhs);
  /* Copy operator */
  Maildir& operator=(const Maildir & rhs);
  /** Equality comparison */
  bool operator==(const Maildir & rhs) const;
  /* Destructor */
  ~Maildir();

  bool isValid() const;
  bool isValid( QString &error ) const;
  QStringList entryList() const;
private:
    void swap( const Maildir& ); 
    class Private;
    Private *d;
};

}
#endif // __MAILDIR_H__
