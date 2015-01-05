/*
 *  kalarmresource.h  -  Akonadi resource for KAlarm
 *  Program:  kalarm
 *  Copyright © 2009-2014 by David Jarvie <djarvie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#ifndef KALARMRESOURCE_H
#define KALARMRESOURCE_H

#include "icalresourcebase.h"

#include <kalarmcal/kacalendar.h>

using namespace KAlarmCal;

class KJob;
namespace Akonadi
{
class CollectionFetchJob;
}
class AlarmTypeRadioWidget;

class KAlarmResource : public ICalResourceBase
{
    Q_OBJECT
public:
    explicit KAlarmResource(const QString &id);
    virtual ~KAlarmResource();

protected:
    /**
     * Customize the configuration dialog before it is displayed.
     */
    void customizeConfigDialog(Akonadi::SingleFileResourceConfigDialog<Akonadi_KAlarm_Resource::Settings> *) Q_DECL_OVERRIDE;
    void configDialogAcceptedActions(Akonadi::SingleFileResourceConfigDialog<Akonadi_KAlarm_Resource::Settings> *) Q_DECL_OVERRIDE;

    void doRetrieveItems(const Akonadi::Collection &) Q_DECL_OVERRIDE;
    bool doRetrieveItem(const Akonadi::Item &, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    bool readFromFile(const QString &fileName) Q_DECL_OVERRIDE;
    bool writeToFile(const QString &fileName) Q_DECL_OVERRIDE;
    void itemAdded(const Akonadi::Item &, const Akonadi::Collection &) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void collectionChanged(const Akonadi::Collection &) Q_DECL_OVERRIDE;
    void retrieveCollections() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void settingsChanged();
    void collectionFetchResult(KJob *);
    void updateFormat(KJob *);
    void setCompatibility(KJob *);

private:
    void checkFileCompatibility(const Akonadi::Collection & = Akonadi::Collection(), bool createAttribute = false);
    Akonadi::CollectionFetchJob *fetchCollection(const char *slot);

    AlarmTypeRadioWidget *mTypeSelector;
    KACalendar::Compat    mCompatibility;
    KACalendar::Compat    mFileCompatibility;  // calendar file compatibility found by readFromFile()
    int                   mVersion;            // calendar format version
    int                   mFileVersion;        // calendar format version found by readFromFile()
    bool                  mHaveReadFile;       // the calendar file has been read
    bool                  mFetchedAttributes;  // attributes have been fetched after initialisation
};

#endif

