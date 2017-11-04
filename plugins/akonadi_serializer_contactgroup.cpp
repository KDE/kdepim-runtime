/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "akonadi_serializer_contactgroup.h"

#include <AkonadiCore/abstractdifferencesreporter.h>
#include <Akonadi/Contact/ContactGroupExpandJob>
#include <AkonadiCore/item.h>

#include <kcontacts/contactgroup.h>
#include <kcontacts/contactgrouptool.h>
#include <KLocalizedString>

#include <qplugin.h>

using namespace Akonadi;

//// ItemSerializerPlugin interface

bool SerializerPluginContactGroup::deserialize(Item &item, const QByteArray &label, QIODevice &data, int version)
{
    Q_UNUSED(label);
    Q_UNUSED(version);

    KContacts::ContactGroup contactGroup;

    if (!KContacts::ContactGroupTool::convertFromXml(&data, contactGroup)) {
        // TODO: error reporting
        return false;
    }

    item.setPayload<KContacts::ContactGroup>(contactGroup);

    return true;
}

void SerializerPluginContactGroup::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version)
{
    Q_UNUSED(label);
    Q_UNUSED(version);

    if (!item.hasPayload<KContacts::ContactGroup>()) {
        return;
    }

    KContacts::ContactGroupTool::convertToXml(item.payload<KContacts::ContactGroup>(), &data);
}

//// DifferencesAlgorithmInterface interface

static bool compareString(const QString &left, const QString &right)
{
    if (left.isEmpty() && right.isEmpty()) {
        return true;
    } else {
        return left == right;
    }
}

static QString toString(const KContacts::Addressee &contact)
{
    return contact.fullEmail();
}

template<class T>
static void compareVector(AbstractDifferencesReporter *reporter, const QString &id, const QVector<T> &left, const QVector<T> &right)
{
    for (int i = 0; i < left.count(); ++i) {
        if (!right.contains(left[ i ])) {
            reporter->addProperty(AbstractDifferencesReporter::AdditionalLeftMode, id, toString(left[ i ]), QString());
        }
    }

    for (int i = 0; i < right.count(); ++i) {
        if (!left.contains(right[ i ])) {
            reporter->addProperty(AbstractDifferencesReporter::AdditionalRightMode, id, QString(), toString(right[ i ]));
        }
    }
}

void SerializerPluginContactGroup::compare(Akonadi::AbstractDifferencesReporter *reporter, const Akonadi::Item &leftItem, const Akonadi::Item &rightItem)
{
    Q_ASSERT(reporter);
    Q_ASSERT(leftItem.hasPayload<KContacts::ContactGroup>());
    Q_ASSERT(rightItem.hasPayload<KContacts::ContactGroup>());

    reporter->setLeftPropertyValueTitle(i18n("Changed Contact Group"));
    reporter->setRightPropertyValueTitle(i18n("Conflicting Contact Group"));

    const KContacts::ContactGroup leftContactGroup = leftItem.payload<KContacts::ContactGroup>();
    const KContacts::ContactGroup rightContactGroup = rightItem.payload<KContacts::ContactGroup>();

    if (!compareString(leftContactGroup.name(), rightContactGroup.name())) {
        reporter->addProperty(AbstractDifferencesReporter::ConflictMode, i18n("Name"),
                              leftContactGroup.name(), rightContactGroup.name());
    }

    // using job->exec() is ok here, not a hot path
    Akonadi::ContactGroupExpandJob *leftJob = new Akonadi::ContactGroupExpandJob(leftContactGroup);
    leftJob->exec();

    Akonadi::ContactGroupExpandJob *rightJob = new Akonadi::ContactGroupExpandJob(rightContactGroup);
    rightJob->exec();

    compareVector(reporter, i18n("Member"), leftJob->contacts(), rightJob->contacts());
}

//// GidExtractorInterface

QString SerializerPluginContactGroup::extractGid(const Item &item) const
{
    if (!item.hasPayload<KContacts::ContactGroup>()) {
        return QString();
    }
    return item.payload<KContacts::ContactGroup>().id();
}
