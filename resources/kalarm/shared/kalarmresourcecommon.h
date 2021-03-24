/*
 *  kalarmresourcecommon.h  -  common functions for KAlarm Akonadi resources
 *  Program:  kalarm
 *  SPDX-FileCopyrightText: 2011-2014 David Jarvie <djarvie@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <kalarmcal/kacalendar.h>
#include <kalarmcal/kaevent.h>

#include <QObject>

namespace KCalendarCore
{
class FileStorage;
}
namespace Akonadi
{
class Collection;
class Item;
}
using namespace KAlarmCal;

namespace KAlarmResourceCommon
{
void initialise(QObject *parent);
//    void          customizeConfigDialog(SingleFileResourceConfigDialog<Settings>*);
KACalendar::Compat getCompatibility(const KCalendarCore::FileStorage::Ptr &, int &version);
Akonadi::Item retrieveItem(const Akonadi::Item &, KAEvent &);
KAEvent checkItemChanged(const Akonadi::Item &, QString &errorMsg);
void setCollectionCompatibility(const Akonadi::Collection &, KACalendar::Compat, int version);

enum ErrorCode { UidNotFound, NotCurrentFormat, EventNotCurrentFormat, EventNoAlarms, EventReadOnly, CalendarAdd };
QString errorMessage(ErrorCode, const QString &param = QString());
}

