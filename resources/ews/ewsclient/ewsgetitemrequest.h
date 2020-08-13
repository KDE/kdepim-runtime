/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSGETITEMREQUEST_H
#define EWSGETITEMREQUEST_H

#include <QList>

#include "ewsitem.h"
#include "ewsitemshape.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class EwsGetItemRequest : public EwsRequest
{
    Q_OBJECT
public:
    class Response : public EwsRequest::Response
    {
    public:
        explicit Response(QXmlStreamReader &reader);
        bool parseItems(QXmlStreamReader &reader);
        const EwsItem &item() const
        {
            return mItem;
        }

    private:
        EwsItem mItem;
    };

    EwsGetItemRequest(EwsClient &client, QObject *parent);
    ~EwsGetItemRequest() override;

    void setItemIds(const EwsId::List &ids);
    void setItemShape(const EwsItemShape &shape);

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
    EwsItemShape mShape;
    QList<Response> mResponses;
};

#endif
