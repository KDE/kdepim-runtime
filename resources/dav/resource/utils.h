/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KDAV/Enums>

#include <Akonadi/Item>

namespace KDAV
{
class DavUrl;
class DavItem;
class DavCollection;
}

namespace Akonadi
{
class Collection;
class Item;
}

/**
 * @short A namespace that contains helper methods for DAV functionality.
 */
namespace Utils
{
/**
 * Returns the i18n'ed name of the given DAV @p protocol dialect.
 */
[[nodiscard]] QString translatedProtocolName(KDAV::Protocol protocol);

/**
 * Creates a new Akonadi::Collection from the KDAV::Collection @p collection.
 */
[[nodiscard]] Akonadi::Collection createAkonadiCollection(const KDAV::DavCollection &davCollection, const Akonadi::Collection &davCollectionRoot);

/**
 * Creates a new KDAV::Collection from the Akonadi::Collection @p collection.
 */
[[nodiscard]] KDAV::DavCollection createDavCollection(const Akonadi::Collection &collection, const KDAV::DavUrl &davUrl);

/**
 * Creates a new KDAV::DavItem from the Akonadi::Item @p item.
 *
 * The returned item will have no payload (DavItem::data() will return an empty
 * QByteArray) if the @p item payload is not recognized.
 */
[[nodiscard]] KDAV::DavItem
createDavItem(const Akonadi::Item &item, const Akonadi::Collection &collection, const Akonadi::Item::List &dependentItems = Akonadi::Item::List());

/**
 * Parses the DAV data contained in @p source and puts it in @p target and @extraItems.
 */
bool parseDavData(const KDAV::DavItem &source, Akonadi::Item &target, Akonadi::Item::List &extraItems);

/**
 * Converts given list of tags into iCal categories
 */
[[nodiscard]] QStringList tagsToCategories(const Akonadi::Tag::List &tags);

/**
 * Determines the DAV protocol from the given collections mimetypes.
 */
[[nodiscard]] std::optional<KDAV::Protocol> protocolFromCollection(const Akonadi::Collection &collection);
}
