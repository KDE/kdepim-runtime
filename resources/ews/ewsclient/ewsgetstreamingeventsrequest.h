/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ewseventrequestbase.h"
#include "ewsid.h"
#include "ewstypes.h"

class EwsGetStreamingEventsRequest : public EwsEventRequestBase
{
    Q_OBJECT
public:
    EwsGetStreamingEventsRequest(EwsClient &client, QObject *parent);
    ~EwsGetStreamingEventsRequest() override;

    void setTimeout(std::chrono::minutes timeout)
    {
        mTimeout = timeout;
    }

    void start() override;
public Q_SLOTS:
    void eventsProcessed(const EwsEventRequestBase::Response &response);
Q_SIGNALS:
    void eventsReceived(KJob *job);
protected Q_SLOTS:
    void requestDataReceived() override;

protected:
    // EWS expects the connection timeout in minutes, between 1 and 30.
    std::chrono::minutes mTimeout;

private:
    /// Returns the size in bytes of the first complete SOAP document in mResponseData, 0 if
    /// none is complete yet, or -1 if the data is not well-formed XML.
    qsizetype completeDocumentSize() const;
    void dumpDocument(const QByteArray &document) const;
};
