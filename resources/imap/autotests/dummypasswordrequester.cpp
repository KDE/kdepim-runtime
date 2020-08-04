/*
   Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   Author: Kevin Ottens <kevin@kdab.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "dummypasswordrequester.h"

#include <QTimer>

#include <QTest>

DummyPasswordRequester::DummyPasswordRequester(QObject *parent)
    : PasswordRequesterInterface(parent)
{
    m_expectedCalls.reserve(10);
    m_results.reserve(10);
    for (int i = 0; i < 10; ++i) {
        m_expectedCalls << StandardRequest;
        m_results << PasswordRetrieved;
    }
}

QString DummyPasswordRequester::password() const
{
    return m_password;
}

void DummyPasswordRequester::setPassword(const QString &password)
{
    m_password = password;
}

void DummyPasswordRequester::setScenario(const QList<RequestType> &expectedCalls, const QList<ResultType> &results)
{
    Q_ASSERT(expectedCalls.size() == results.size());

    m_expectedCalls = expectedCalls;
    m_results = results;
}

void DummyPasswordRequester::setDelays(const QList<int> &delays)
{
    m_delays = delays;
}

void DummyPasswordRequester::requestPassword(RequestType request, const QString & /*serverError*/)
{
    QVERIFY2(!m_expectedCalls.isEmpty(), QStringLiteral("Got unexpected call: %1").arg(request).toUtf8().constData());
    QCOMPARE((int)request, (int)m_expectedCalls.takeFirst());

    int delay = 20;
    if (!m_delays.isEmpty()) {
        delay = m_delays.takeFirst();
    }

    QTimer::singleShot(delay, this, &DummyPasswordRequester::emitResult);
}

void DummyPasswordRequester::emitResult()
{
    ResultType result = m_results.takeFirst();

    if (result == PasswordRetrieved) {
        Q_EMIT done(result, m_password);
    } else {
        Q_EMIT done(result);
    }
}
