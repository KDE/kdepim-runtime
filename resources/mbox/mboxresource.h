/*
    Copyright (c) 2009 Bertjan Broeksem <broeksema@kde.org>

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

#ifndef MBOX_RESOURCE_H
#define MBOX_RESOURCE_H

#include "settings.h"
#include "singlefileresource.h"

namespace KMBox
{
class MBox;
}

class MboxResource : public Akonadi::SingleFileResource<Settings>
{
    Q_OBJECT

public:
    explicit MboxResource(const QString &id);
    ~MboxResource();

protected Q_SLOTS:
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &col) Q_DECL_OVERRIDE;

protected:
    void aboutToQuit() Q_DECL_OVERRIDE;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void itemRemoved(const Akonadi::Item &item) Q_DECL_OVERRIDE;

    // From SingleFileResourceBase
    void handleHashChange() Q_DECL_OVERRIDE;
    bool readFromFile(const QString &fileName) Q_DECL_OVERRIDE;
    bool writeToFile(const QString &fileName) Q_DECL_OVERRIDE;
    /**
     * Customize the configuration dialog before it is displayed.
     */
    void customizeConfigDialog(Akonadi::SingleFileResourceConfigDialog<Settings> *dlg) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onCollectionFetch(KJob *job);
    void onCollectionModify(KJob *job);

private:
    QHash<KJob *, Akonadi::Item> mCurrentItemDeletions;
    KMBox::MBox *mMBox;
};

#endif
