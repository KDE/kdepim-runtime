/*
    SPDX-FileCopyrightText: 2016 Stefan Stäglich <sstaeglich@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef O1TOMBOY_H
#define O1TOMBOY_H

#include "o2/o1.h"

class O1Tomboy : public O1
{
    Q_OBJECT
public:
    explicit O1Tomboy(QObject *parent = nullptr);

    void setBaseURL(const QString &value);
    QString getRequestToken() const;

    QString getRequestTokenSecret() const;

    void restoreAuthData(const QString &token, const QString &secret);
};

#endif // O1TOMBOY_H
