/*
 *  kalarmresource.h  -  Akonadi resource for KAlarm
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2009-2020 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "icalresourcebase.h"

#include <KAlarmCal/KACalendar>

using namespace KAlarmCal;

class KJob;
namespace Akonadi
{
class CollectionFetchJob;
}

class KAlarmResource : public ICalResourceBase
{
    Q_OBJECT
public:
    explicit KAlarmResource(const QString &id);
    ~KAlarmResource() override;

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual
    void retrieveItems(const Akonadi::Collection &) override;
    void doRetrieveItems(const Akonadi::Collection &) override;
    bool doRetrieveItems(const Akonadi::Item::List &items, const QSet<QByteArray> &parts) override;
    bool readFromFile(const QString &fileName) override;
    bool writeToFile(const QString &fileName) override;
    void itemAdded(const Akonadi::Item &, const Akonadi::Collection &) override;
    void itemChanged(const Akonadi::Item &, const QSet<QByteArray> &parts) override;
    void collectionChanged(const Akonadi::Collection &) override;
    void retrieveCollections() override;
    bool readOnly() const override;

private Q_SLOTS:
    void settingsChanged();
    void collectionFetchResult(KJob *);
    void updateFormat(KJob *);
    void setCompatibility(KJob *);

private:
    void checkFileCompatibility(const Akonadi::Collection & = Akonadi::Collection(), bool createAttribute = false);
    Akonadi::CollectionFetchJob *fetchCollection(const char *slot);

    KACalendar::Compat mCompatibility;
    KACalendar::Compat mFileCompatibility; // calendar file compatibility found by readFromFile()
    int mVersion; // calendar format version
    int mFileVersion; // calendar format version found by readFromFile()
    bool mHaveReadFile{false}; // the calendar file has been read
    bool mFetchedAttributes{false}; // attributes have been fetched after initialisation
    bool mUpdatingFormat{false}; // writeToFile() can ignore mCompatibility
    bool mFileChangedReadOnly{false}; // make readOnly() return true
    QString mBackupFile; // backup file name, if another process has changed the calendar file
};

