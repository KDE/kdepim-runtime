/*  -*- mode: C++; c-file-style: "gnu" -*-

    This file is part of kdepim.
    Copyright (c) 2004 KDEPIM developers

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef EMAIL_H
#define EMAIL_H

#include <qstringlist.h>
#include <qcstring.h>

namespace KPIM {

// Helper functions
/** Split a comma separated list of email addresses. */
QStringList splitEmailAddrList(const QString& aStr);

/** Return email address from string. Examples:
 * "Stefan Taferner <taferner@kde.org>" returns "taferner@kde.org"
 * "joe@nowhere.com" returns "joe@nowhere.com". Note that this only
 * returns the first address. */
QCString getEmailAddr(const QString& aStr);

/** Return email address and name from string. Examples:
 * "Stefan Taferner <taferner@kde.org>" returns "taferner@kde.org"
 * and "Stefan Taferner". "joe@nowhere.com" returns "joe@nowhere.com"
 * and "". Note that this only returns the first address.
 * Also note that the return value is TRUE if both the name and the
 * mail are not empty: this does NOT tell you if mail contains a
 * valid email address or just some rubbish.
 */
bool getNameAndMail(const QString& aStr, QString& name, QString& mail);

/**
 * Compare two email addresses. If matchName is false, it just checks
 * the email address, and returns true if this matches. If matchName
 * is true, both the name and the email must be the same.
 */
bool compareEmail( const QString& email1, const QString& email2,
                   bool matchName );

} // namespace

#endif /* EMAIL_H */

