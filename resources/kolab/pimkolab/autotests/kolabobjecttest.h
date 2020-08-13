/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef KOLABOBJECTTEST_H
#define KOLABOBJECTTEST_H
#include <QObject>

class KolabObjectTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void preserveLatin1();
    void preserveUnicode();
    void dontCrashWithEmptyOrganizer();
    void dontCrashWithEmptyIncidence();
    void parseRelationMembers();
};

#endif // KOLABOBJECTTEST_H
