/*
    Copyright (C) 2020  Igor Poboiko <igor.poboiko@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GENERICHANDLER_H
#define GENERICHANDLER_H

#include <KGAPI/Types>

#include <QObject>
#include <QSharedPointer>

#include <AkonadiCore/Item>
#include <AkonadiCore/Collection>

#include <KI18n/KLocalizedString>

#include "googleresourcestateinterface.h"

#define ITEM_PROPERTY "_AkonadiItem"
#define ITEMS_PROPERTY "_AkonadiItems"
#define COLLECTION_PROPERTY "_AkonadiCollection"

namespace KGAPI2 {
    class Job;
}

class GoogleSettings;

class GenericHandler : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<GenericHandler> Ptr;

    GenericHandler(GoogleResourceStateInterface *iface, GoogleSettings *settings);
    virtual ~GenericHandler();

    virtual QString mimeType() = 0;

    virtual void retrieveCollections(const Akonadi::Collection &rootCollection) = 0;
    virtual void retrieveItems(const Akonadi::Collection &collection) = 0;

    virtual void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) = 0;
    virtual void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) = 0;
    virtual void itemsRemoved(const Akonadi::Item::List &items) = 0;
    virtual void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) = 0;
    virtual void itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);
    virtual void itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);

    virtual void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) = 0;
    virtual void collectionChanged(const Akonadi::Collection &collection) = 0;
    virtual void collectionRemoved(const Akonadi::Collection &collection) = 0;

    /*
     * Helper function for various handlers
     */
    template<typename T>
    bool canPerformTask(const Akonadi::Item &item)
    {
        if (item.isValid() && (!item.hasPayload<T>() || item.mimeType() != mimeType())) {
            m_iface->cancelTask(i18n("Invalid item."));
            return false;
        }
        return m_iface->canPerformTask();
    }

    template<typename T>
    bool canPerformTask(const Akonadi::Item::List &items)
    {
        if (std::any_of(items.cbegin(), items.cend(), [this](const Akonadi::Item &item){
                    return item.isValid() && (!item.hasPayload<T>() || item.mimeType() != mimeType());
                })) {
            m_iface->cancelTask(i18n("Invalid item."));
            return false;
        }
        return m_iface->canPerformTask();
    }

    virtual bool canPerformTask(const Akonadi::Item &item) = 0;
    virtual bool canPerformTask(const Akonadi::Item::List &items) = 0;
protected Q_SLOTS:
    void slotGenericJobFinished(KGAPI2::Job *job);
protected:
    void emitReadyStatus();

    GoogleResourceStateInterface *m_iface = nullptr;
    GoogleSettings *m_settings = nullptr;
};

#endif // GENERICHANDLER_H
