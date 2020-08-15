/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BENCHMARK_TEST_H
#define BENCHMARK_TEST_H

#include <QObject>

class BenchmarkTests : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void parsingBenchmarkComparison_data();
    void parsingBenchmarkComparison();
};

#endif
