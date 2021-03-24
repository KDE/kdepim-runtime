/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <AkonadiAgentBase/ResourceBase>

#include "graph.h"

class KJob;

class FacebookResource : public Akonadi::ResourceBase
{
    Q_OBJECT

public:
    explicit FacebookResource(const QString &id);
    ~FacebookResource() override;

    void abortActivity() override;
    void cleanup() override;

    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;

private Q_SLOTS:
    void onListJobDone(KJob *job);

private:
    Akonadi::Collection makeCollection(Graph::RSVP rsvp, const QString &name, const Akonadi::Collection &parent);

private:
    KJob *mCurrentJob = nullptr;
};

