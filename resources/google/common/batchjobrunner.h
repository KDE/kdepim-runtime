/*
    Copyright (C) 2018  Daniel Vrátil <dvratil@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BATCHJOBRUNNER_H
#define BATCHJOBRUNNER_H

#include <functional>

#include <KGAPI/Job>

#include <QTimer>

template<class Entities>
class BatchJobRunner
{
public:
    using EntityCb = std::function<KGAPI2::Job*(typename Entities::const_reference)>;
    using DoneCb = std::function<void(const Entities &)>;

    BatchJobRunner(const Entities &entities,
                   EntityCb &&entityCb, DoneCb &&doneCb)
        : mEntities(entities), mDoneCb(std::move(doneCb))
    {
        for (const auto &entity : entities) {
            auto job = entityCb(entity);
            QObject::connect(job, &KGAPI2::Job::finished,
                             [this](auto job) {
                                mJobs.remove(job);
                                checkDone();
                             });
            mJobs.insert(job);
        }

        checkDone();
    }

    BatchJobRunner(const BatchJobRunner<Entities> &other) = delete;
    BatchJobRunner(BatchJobRunner<Entities> &&other) = delete;

private:
    void checkDone() {
        if (mJobs.isEmpty()) {
            mDoneCb(mEntities);
            QTimer::singleShot(0, [this]() { delete this; });
        }
    }

    Entities mEntities;
    QSet<KGAPI2::Job*> mJobs;
    DoneCb mDoneCb;
};


#endif


