/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Tobias Koenig <tokoe@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SENTACTIONHANDLER_H
#define SENTACTIONHANDLER_H

#include <MailTransportAkonadi/SentActionAttribute>

#include <QObject>

class KJob;

class SentActionHandler : public QObject
{
    Q_OBJECT

public:
    explicit SentActionHandler(QObject *parent = nullptr);

    void runAction(const MailTransport::SentActionAttribute::Action &action);

private Q_SLOTS:
    void itemFetchResult(KJob *job);
};

#endif
