/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSMOVEITEMREQUEST_H
#define EWSMOVEITEMREQUEST_H

#include <QList>
#include <QSharedPointer>

#include "ewsitem.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;

class EwsMoveItemRequest : public EwsRequest
{
    Q_OBJECT
public:
    class Response : public EwsRequest::Response
    {
    public:
        const EwsId &itemId() const
        {
            return mId;
        }

    protected:
        Response(QXmlStreamReader &reader);

        EwsId mId;

        friend class EwsMoveItemRequest;
    };

    EwsMoveItemRequest(EwsClient &client, QObject *parent);
    ~EwsMoveItemRequest() override;

    void setItemIds(const EwsId::List &ids)
    {
        mIds = ids;
    }

    void setDestinationFolderId(const EwsId &id)
    {
        mDestFolderId = id;
    }

    void start() override;

    const QList<Response> &responses() const
    {
        return mResponses;
    }

protected:
    bool parseResult(QXmlStreamReader &reader) override;
    bool parseItemsResponse(QXmlStreamReader &reader);

private:
    EwsId::List mIds;
    EwsId mDestFolderId;
    QList<Response> mResponses;
};

#endif
