/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef VCARDRESOURCE_H
#define VCARDRESOURCE_H

#include "settings.h"
#include "singlefileresource.h"

#include <kcontacts/addressee.h>
#include <kcontacts/vcardconverter.h>

class VCardResource : public Akonadi::SingleFileResource<Akonadi_VCard_Resource::Settings>
{
    Q_OBJECT

public:
    explicit VCardResource(const QString &id);
    ~VCardResource() override;

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected Q_SLOTS:
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void retrieveItems(const Akonadi::Collection &col) override;

protected:
    bool readFromFile(const QString &fileName) override;
    bool writeToFile(const QString &fileName) override;
    void aboutToQuit() override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

private:
    QMap<QString, KContacts::Addressee> mAddressees;
    KContacts::VCardConverter mConverter;
};

#endif
