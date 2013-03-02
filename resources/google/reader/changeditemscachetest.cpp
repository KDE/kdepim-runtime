/*
    Copyright (C) 2013  Daniel Vrátil <dan@progdan.cz>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <QTextStream>

#include "changeditemscache.h"

int main(int argc, char **argv)
{
    QTextStream cin(stdin);
    QTextStream cout(stdout);

    ChangedItemsCache cache("testResource");

    ChangedItemsCache::CacheItem::List items = cache.items();
    cout << "== Cache contains " << items.count() << " items ==\n\n";
    cout.flush();
    int i = 0;
    Q_FOREACH (const ChangedItemsCache::CacheItem &item, items) {

        cout << "== Item " << i << " ==\n"
             << "Akonadi Item ID: " << item.id << "\n"
             << "Feed ID: " << item.feedId << "\n"
             << "Item ID: " << item.itemId << "\n";
        cout.flush();

        i++;
    }

    while (true) {
        cout << "\n\nAdd new item to cache? (Y/N) ";
        cout.flush();
        QString i = cin.readLine();

        if (i.toLower() == "n") {
            break;
        }

        ChangedItemsCache::CacheItem item;
        cout << "Akonadi Item ID: ";
        cout.flush();
        item.id = cin.readLine().toLong();
        cout << "Feed ID: ";
        cout.flush();
        item.feedId = cin.readLine();
        cout << "Item ID: ";
        cout.flush();
        item.itemId = cin.readLine();
        cout << "Added tags (comma separated): ";

        cache.addItem(item);
        cout << "== Item added to cache ==\n";
        cout.flush();
    }

    cout << "\n\n== Cache saved, quiting ==\n\n";
    cout.flush();

    return 0;
}