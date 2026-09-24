/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <QString>
#include <QXmlStreamReader>

class PushNotificationMessageParser
{
public:
    PushNotificationMessageParser(const QByteArray &data);

    bool isContentUpdate = false;
    bool isPropertyUpdate = false;
    bool isVapidKeyUpdate = false;
    bool isValid = true;
    QString topic;
    QString syncToken;

private:
    void readPushMessage();
    void readContentUpdate();
    void readPropertyUpdate();
    void readPropertyProp();
    void readTopic();
    QXmlStreamReader m_reader;
    bool m_isTransport = false;
};
