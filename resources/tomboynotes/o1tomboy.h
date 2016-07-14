/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>

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

#ifndef O1TOMBOY_H
#define O1TOMBOY_H

#include "o2/o1.h"

class O1Tomboy : public O1
{
    Q_OBJECT
public:
    explicit O1Tomboy(QObject *parent = Q_NULLPTR);

    void setBaseURL(const QString &value);
    QString getRequestToken() const;

    QString getRequestTokenSecret() const;

    void restoreAuthData(const QString &token, const QString &secret);
};

#endif // O1TOMBOY_H
