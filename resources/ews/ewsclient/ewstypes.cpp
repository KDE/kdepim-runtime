/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewstypes.h"

const QString soapEnvNsUri = QStringLiteral("http://schemas.xmlsoap.org/soap/envelope/");
const QString ewsMsgNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/messages");
const QString ewsTypeNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/types");

const QVector<QString> ewsItemTypeNames = {
    QStringLiteral("Item"),
    QStringLiteral("Message"),
    QStringLiteral("CalendarItem"),
    QStringLiteral("Contact"),
    QStringLiteral("DistributionList"),
    QStringLiteral("MeetingMessage"),
    QStringLiteral("MeetingRequest"),
    QStringLiteral("MeetingResponse"),
    QStringLiteral("MeetingCancellation"),
    QStringLiteral("Task")
};
