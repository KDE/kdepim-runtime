/*
    Copyright (C) 2017 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef TEST_STATEMONITOR_H
#define TEST_STATEMONITOR_H

#include <functional>

#include <QObject>
#include <QTimer>

#include <AkonadiCore/Collection>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiCore/Monitor>

class StateMonitorBase : public QObject
{
    Q_OBJECT
public:
    explicit StateMonitorBase(QObject *parent) : QObject(parent)
    {
    }

    ~StateMonitorBase() override = default;
Q_SIGNALS:
    void stateReached();
    void errorOccurred();
};

template<typename T> class CollectionStateMonitor : public StateMonitorBase
{
public:
    typedef std::function<bool (const Akonadi::Collection &col, const T &state)> StateComparisonFunc;
    CollectionStateMonitor(QObject *parent, const QHash<QString, T> &stateHash, const QString &inboxId, const StateComparisonFunc &comparisonFunc, int recheckInterval = 0);
    ~CollectionStateMonitor() override = default;
    Akonadi::Monitor &monitor()
    {
        return mMonitor;
    }

    void forceRecheck();
private:
    void stateChanged(const Akonadi::Collection &col);

    Akonadi::Monitor mMonitor;
    QSet<QString> mPending;
    const QHash<QString, T> &mStateHash;
    StateComparisonFunc mComparisonFunc;
    const QString &mInboxId;
    QTimer mRecheckTimer;
};

template<typename T>
CollectionStateMonitor<T>::CollectionStateMonitor(QObject *parent, const QHash<QString, T> &stateHash, const QString &inboxId, const StateComparisonFunc &comparisonFunc, int recheckInterval)
    : StateMonitorBase(parent)
    , mMonitor(this)
    , mPending(stateHash.keys().toSet())
    , mStateHash(stateHash)
    , mComparisonFunc(comparisonFunc)
    , mInboxId(inboxId)
    , mRecheckTimer(this)
{
    connect(&mMonitor, &Akonadi::Monitor::collectionAdded, this,
            [this](const Akonadi::Collection &col, const Akonadi::Collection &) {
        stateChanged(col);
    });
    connect(&mMonitor, QOverload<const Akonadi::Collection &>::of(&Akonadi::Monitor::collectionChanged), this,
            [this](const Akonadi::Collection &col) {
        stateChanged(col);
    });
    if (recheckInterval > 0) {
        mRecheckTimer.setInterval(recheckInterval);
        connect(&mRecheckTimer, &QTimer::timeout, this, &CollectionStateMonitor::forceRecheck);
        mRecheckTimer.start();
    }
}

template<typename T>
void CollectionStateMonitor<T>::stateChanged(const Akonadi::Collection &col)
{
    auto remoteId = col.remoteId();
    auto state = mStateHash.find(remoteId);
    if (state == mStateHash.end()) {
        qDebug() << "Cannot find state for collection" << remoteId;
        Q_EMIT errorOccurred();
    }
    if (mComparisonFunc(col, *state)) {
        mPending.remove(remoteId);
    } else {
        mPending.insert(remoteId);
    }
    if (mPending.empty()) {
        Q_EMIT stateReached();
    }
}

template<typename T>
void CollectionStateMonitor<T>::forceRecheck()
{
    auto fetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(), Akonadi::CollectionFetchJob::Recursive, this);
    fetchJob->setFetchScope(mMonitor.collectionFetchScope());
    if (fetchJob->exec()) {
        for (const auto &col : fetchJob->collections()) {
            const auto remoteId = col.remoteId();
            const auto state = mStateHash.find(remoteId);
            if (state != mStateHash.end()) {
                stateChanged(col);
            }
        }
    }
}

#endif
