/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "davpushnotifybridge.h"
#include "davpushnotifybridge_debug.h"

#include "kunifiedpush_version.h"

#include <QCoreApplication>
#include <QDBusConnection>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));

    QCoreApplication app(argc, argv);

    KAboutData about(u"akonadi_davpushnotifybridge"_s, QString(), QString::fromLatin1(KUNIFIEDPUSH_VERSION_STRING));
    KAboutData::setApplicationData(about);

    KCrash::initialize();

    DavPushNotifyBridge const bridge;
    KDBusService service(KDBusService::Unique);

    qCDebug(DAVPUSHNOTIFYBRIDGE_LOG) << "Starting DAV push notify bridge";
    return app.exec();
}
