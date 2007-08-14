/*
    This file is part of libkdepim.

    Copyright (c) 2006 Christian Schaarschmidt <schaarsc@gmx.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KPIM_KMAILCOMPLETION_H
#define KPIM_KMAILCOMPLETION_H

#include <QSet>
#include <qstringlist.h>
#include "kcompletion.h"


namespace KPIM {

/**
 * KMailCompletion allows lookup of email addresses by keyword.
 * Typically a keywods would be firstname, lastname, nickname or domain. 
 */
class KMailCompletion : public KCompletion
{
  Q_OBJECT

  public:
    KMailCompletion();

    /**
     * clears internal keyword map and calls KCompletion::clear.
     */
    virtual void clear();

    /**
     * uses KCompletion::makeCompletion to find email addresses which starts with string.
     * ignores keywords.
     *
     * @returns email address
     */
    QString makeCompletion( const QString &string );

    /**
     * specify keywords for email.
     *
     * Items may be added with KCompletion::addItem, those will only be returned as match if they
     * are in one of these formats:
     * \li contains localpart@domain 
     * \li contains <email>
     * or if they have also been added with this function.
     */
    void addItemWithKeys( const QString& email, int weight, const QStringList * keyWords);

    /**
     * use internal map to replace all keywords in pMatches with corresponding email addresses.
     */
    virtual void postProcessMatches( QStringList * pMatches )const;

  private:
    QMap< QString, QStringList > m_keyMap;
};

}

#endif
