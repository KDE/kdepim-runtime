/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSSUBSCRIBEDFOLDERSJOB_H
#define EWSSUBSCRIBEDFOLDERSJOB_H

#include "ewsjob.h"
#include "ewsid.h"

class EwsClient;
class EwsSettings;

class EwsSubscribedFoldersJob : public EwsJob
{
    Q_OBJECT
public:
    EwsSubscribedFoldersJob(EwsClient &client, EwsSettings *settings, QObject *parent);
    ~EwsSubscribedFoldersJob() override;

    void start() override;

    EwsId::List folders()
    {
        return mFolders;
    }

    static const EwsId::List &defaultSubscriptionFolders();
private Q_SLOTS:
    void verifySubFoldersRequestFinished(KJob *job);
private:
    EwsId::List mFolders;
    EwsClient &mClient;
    EwsSettings *mSettings = nullptr;
};

#endif
