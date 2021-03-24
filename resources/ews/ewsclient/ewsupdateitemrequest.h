/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QList>
#include <QSharedPointer>

#include "ewsitem.h"
#include "ewsitemshape.h"
#include "ewsrequest.h"
#include "ewstypes.h"

class QXmlStreamReader;
class QXmlStreamWriter;

class EwsUpdateItemRequest : public EwsRequest
{
    Q_OBJECT
public:
    class Update
    {
    public:
        bool write(QXmlStreamWriter &writer, EwsItemType itemType) const;

    protected:
        enum Type { Append = 0, Set, Delete, Unknown };

        Update(const EwsPropertyField &field, const QVariant &val, Type type)
            : mField(field)
            , mValue(val)
            , mType(type)
        {
        }

        EwsPropertyField mField;
        QVariant mValue;
        Type mType;
    };

    class AppendUpdate : public Update
    {
    public:
        AppendUpdate(const EwsPropertyField &field, const QVariant &val)
            : Update(field, val, Append)
        {
        }
    };

    class SetUpdate : public Update
    {
    public:
        SetUpdate(const EwsPropertyField &field, const QVariant &val)
            : Update(field, val, Set)
        {
        }
    };

    class DeleteUpdate : public Update
    {
    public:
        DeleteUpdate(const EwsPropertyField &field)
            : Update(field, QVariant(), Delete)
        {
        }
    };

    class ItemChange
    {
    public:
        ItemChange(const EwsId &itemId, EwsItemType type)
            : mId(itemId)
            , mType(type)
        {
        }

        void addUpdate(const Update *upd)
        {
            mUpdates.append(QSharedPointer<const Update>(upd));
        }

        bool write(QXmlStreamWriter &writer) const;

    private:
        EwsId mId;
        EwsItemType mType;
        QList<QSharedPointer<const Update>> mUpdates;
    };

    class Response : public EwsRequest::Response
    {
    public:
        const EwsId &itemId() const
        {
            return mId;
        }

        unsigned conflictCount() const
        {
            return mConflictCount;
        }

    protected:
        Response(QXmlStreamReader &reader);

        unsigned mConflictCount;
        EwsId mId;

        friend class EwsUpdateItemRequest;
    };

    EwsUpdateItemRequest(EwsClient &client, QObject *parent);
    ~EwsUpdateItemRequest() override;

    void addItemChange(const ItemChange &change)
    {
        mChanges.append(change);
    }

    void setMessageDisposition(EwsMessageDisposition disp)
    {
        mMessageDisp = disp;
    }

    void setConflictResolution(EwsConflictResolution resol)
    {
        mConflictResol = resol;
    }

    void setMeetingDisposition(EwsMeetingDisposition disp)
    {
        mMeetingDisp = disp;
    }

    void setSavedFolderId(const EwsId &id)
    {
        mSavedFolderId = id;
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
    QList<ItemChange> mChanges;
    EwsMessageDisposition mMessageDisp;
    EwsConflictResolution mConflictResol;
    EwsMeetingDisposition mMeetingDisp;
    EwsId mSavedFolderId;
    QList<Response> mResponses;
};

