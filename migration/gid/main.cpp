/*
    Copyright (c) 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <AkonadiWidgets/ControlGui>
#include <kaboutdata.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <KLocalizedString>

#include <QDebug>
#include <QIcon>
#include <QApplication>
#include <infodialog.h>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "gidmigrator.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("gid-migrator");
    KAboutData aboutData(QStringLiteral("gid-migrator"),
                         i18n("GID Migration Tool"),
                         QStringLiteral("0.1"),
                         i18n("Migration of Akonadi Items to support GID"),
                         KAboutLicense::LGPL,
                         i18n("(c) 2013-2016 the Akonadi developers"),
                         QStringLiteral("http://pim.kde.org/akonadi/"));
    aboutData.addAuthor(i18n("Christian Mollekopf"), i18n("Author"), QStringLiteral("mollekopf@kolabsys.com"));

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("interactive"), i18n("Show reporting dialog")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("interactive-on-change"), i18n("Show report only if changes were made")));
    parser.addOption(QCommandLineOption(QStringLiteral("mimetype"), i18n("MIME type to migrate"), QStringLiteral("mimetype")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("akonadi")));

    if (!Akonadi::ControlGui::start(nullptr)) {
        return 2;
    }

    InfoDialog *infoDialog = nullptr;
    if (parser.isSet(QStringLiteral("interactive")) || parser.isSet(QStringLiteral("interactive-on-change"))) {
        infoDialog = new InfoDialog(parser.isSet(QStringLiteral("interactive-on-change")));
        Akonadi::ControlGui::widgetNeedsAkonadi(infoDialog);
        infoDialog->show();
    }

    const QString mimeType = parser.value(QStringLiteral("mimetype"));
    if (mimeType.isEmpty()) {
        qWarning() << "set the mimetype to migrate";
        return 5;
    }

    GidMigrator *migrator = new GidMigrator(mimeType);
    if (infoDialog && migrator) {
        infoDialog->migratorAdded();
        QObject::connect(migrator, &MigratorBase::message,
                         infoDialog, QOverload<MigratorBase::MessageType, const QString &>::of(&InfoDialog::message));
        QObject::connect(migrator, &QObject::destroyed, infoDialog, &InfoDialog::migratorDone);
        QObject::connect(migrator, QOverload<int>::of(&MigratorBase::progress), infoDialog, QOverload<int>::of(&InfoDialog::progress));
    }
    QObject::connect(migrator, &GidMigrator::stoppedProcessing, &app, &QApplication::quit);
    migrator->start();
    const int result = app.exec();
    if (InfoDialog::hasError() || migrator->migrationState() == MigratorBase::Failed) {
        return 3;
    }

    return result;
}
