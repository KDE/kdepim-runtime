/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <AkonadiWidgets/ControlGui>
#include <KAboutData>

#include <KConfig>
#include <KLocalizedString>

#include "migration_debug.h"
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
                         i18n("(c) 2013-2020 the Akonadi developers"),
                         QStringLiteral("https://community.kde.org/KDE_PIM/Akonadi"));
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
        qCWarning(MIGRATION_LOG) << "set the mimetype to migrate";
        return 5;
    }

    auto migrator = new GidMigrator(mimeType);
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
