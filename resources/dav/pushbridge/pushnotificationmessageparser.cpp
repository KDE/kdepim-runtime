/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pushnotificationmessageparser.h"

using namespace Qt::Literals;

PushNotificationMessageParser::PushNotificationMessageParser(const QByteArray &data)
{
    m_reader.addData(data);
    if (m_reader.readNextStartElement()) {
        if (m_reader.name() == "push-message"_L1) {
            readPushMessage();
        } else {
            isValid = false;
        }
    } else {
        isValid = false;
    }
    if (isValid && m_isTransport && !isContentUpdate) {
        isVapidKeyUpdate = true;
        isPropertyUpdate = false; // Set to false as we want to keep only vapidKeyUpdate flag
    }
}

void PushNotificationMessageParser::readPushMessage()
{
    Q_ASSERT(m_reader.isStartElement() && m_reader.name() == "push-message"_L1);
    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "topic"_L1) {
            readTopic();
        } else if (m_reader.name() == "content-update"_L1) {
            readContentUpdate();
        } else if (m_reader.name() == "property-update"_L1) {
            readPropertyUpdate();
        } else {
            m_reader.skipCurrentElement();
        }
    }
}

void PushNotificationMessageParser::readContentUpdate()
{
    Q_ASSERT(m_reader.isStartElement() && m_reader.name() == "content-update"_L1);
    isContentUpdate = true;
    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "sync-token"_L1) {
            syncToken = m_reader.readElementText();
        } else {
            m_reader.skipCurrentElement();
        }
    }
}

void PushNotificationMessageParser::readPropertyUpdate()
{
    Q_ASSERT(m_reader.isStartElement() && m_reader.name() == "property-update"_L1);
    isPropertyUpdate = true;
    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "prop"_L1) {
            readPropertyProp();
        }
    }
    m_reader.skipCurrentElement();
}

void PushNotificationMessageParser::readPropertyProp()
{
    Q_ASSERT(m_reader.isStartElement() && m_reader.name() == "prop"_L1);
    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "transports"_L1) {
            m_isTransport = true;
        }
        m_reader.skipCurrentElement();
    }
}

void PushNotificationMessageParser::readTopic()
{
    Q_ASSERT(m_reader.isStartElement() && m_reader.name() == "topic"_L1);
    topic = m_reader.readElementText();
}
