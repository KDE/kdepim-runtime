/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Akonadi/Collection>
#include <KCompositeJob>
#include <KDAV/DavPushRegistration>

class Settings;

namespace KDAV
{
class DavPushRegistration;
}

/**
 * Handles the registration and AkonadiServer updates of collections supporting
 * DavPush. Provided collections must :
 * - be valid,
 * - have a DavPushRegistration and a valid topic.
 *
 * The collections are registered using a KDAV::DavPushRegistrationJob, and
 * upon successful registration updates the DavPushAttribute in AkonadiServer.
 *
 * The job fails if any registration or update fails.
 */
class DavPushRegisterCollectionJobs : public KCompositeJob
{
    Q_OBJECT

public:
    /**
     * Creates a DavPushRegisterCollectionJobs registering provided @p collections with
     * @p davPushRegistration, and using @p settings to compute DavUrls.
     */
    DavPushRegisterCollectionJobs(const QList<Akonadi::Collection> &collections,
                                  const KDAV::DavPushRegistration &davPushRegistration,
                                  Settings *settings,
                                  QObject *parent = nullptr);

    void start() override;

private:
    void slotResult(KJob *job) override;

    Akonadi::Collection::List mCollections;
    KDAV::DavPushRegistration mDavPushRegistration;
    Settings *mSettings;
};
