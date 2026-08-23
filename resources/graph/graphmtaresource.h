/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Separate mail-transport agent. KMail requires that a single Akonadi resource does not
    both send and receive mail, so sending lives in its own executable (cf. ewsmtaresource).

    Like the EWS MTA it does not talk to the server itself: the MIME content is forwarded
    over D-Bus to the master Graph resource (sendMessage/messageSent), so only one process
    performs OAuth and holds tokens.
*/

#pragma once

#include <Akonadi/Item>
#include <Akonadi/ResourceWidgetBase>
#include <Akonadi/TransportResourceBase>

#include <QHash>

class OrgKdeAkonadiGraphResourceInterface;

class GraphMtaResource : public Akonadi::ResourceWidgetBase, public Akonadi::TransportResourceBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.GraphMta.Resource")
public:
    explicit GraphMtaResource(const QString &id);
    ~GraphMtaResource() override;

    void sendItem(const Akonadi::Item &item) override;

protected Q_SLOTS:
    void retrieveCollections() override
    {
    } // transport-only: no collections
    void retrieveItems(const Akonadi::Collection &) override
    {
    }
    bool retrieveItems(const Akonadi::Item::List &, const QSet<QByteArray> &) override
    {
        return false;
    }

private:
    void messageSent(const QString &id, const QString &error);
    [[nodiscard]] bool connectToMasterResource();

    OrgKdeAkonadiGraphResourceInterface *mGraphResource = nullptr;
    QHash<QString, Akonadi::Item> mItemHash; // remoteId -> item, awaiting messageSent
};
