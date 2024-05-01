/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kolabresource.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotification>
#include <KSharedConfig>
#include <QIcon>

using namespace Akonadi;
using namespace Qt::StringLiterals;

KolabResource::KolabResource(const QString &id)
    : ResourceBase(id)
{
    auto config = KSharedConfig::openStateConfig();
    auto group = config->group(u"General"_s);
    auto show = group.readEntry<bool>(u"ShowNotification-"_s + id, true);
    if (show) {
        auto ntf = KNotification::event(QStringLiteral("deprecated"),
                                        i18nc("@title",
                                              "The Kolab support in KMail was removed. Please migrate your account to "
                                              "use the standard IMAP and WebDAV module."),
                                        {},
                                        KNotification::Persistent | KNotification::SkipGrouping);
        ntf->setComponentName(QStringLiteral("akonadi_kolab_resource"));
        connect(ntf, &KNotification::ignored, ntf, &KNotification::close);
        ntf->sendEvent();

        group.writeEntry(u"ShowNotification-"_s + id, false);
        config->sync();
    }

    setAgentName(i18nc("@title", "Kolab Resource (obsolete)"));
    setNeedsNetwork(false);
    Q_EMIT status(NotConfigured, i18nc("@info", "The Kolab resource in KMail was removed."));
}

KolabResource::~KolabResource() = default;

void KolabResource::retrieveCollections()
{
}

void KolabResource::retrieveItems(const Collection &collection)
{
}

bool KolabResource::retrieveItems(const Item::List &items, const QSet<QByteArray> &parts)
{
    return true;
}

AKONADI_RESOURCE_CORE_MAIN(KolabResource)

#include "moc_kolabresource.cpp"
