/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FACEBOOK_RESOURCE_H_
#define FACEBOOK_RESOURCE_H_

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

#endif
