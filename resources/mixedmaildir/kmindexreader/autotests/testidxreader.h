/*
 *   SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
 *   SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef TESTIDXREADER_H
#define TESTIDXREADER_H

#include <QObject>

class TestIdxReader : public QObject
{
    Q_OBJECT
public:
    TestIdxReader();
private Q_SLOTS:
    void testError();
    void testReadHeader();
    void testRead();
private:
};

#endif // TESTIDXREADER_H
