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

#include <AkonadiCore/Control>
#include <kaboutdata.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <klocale.h>

#include <qdebug.h>
#include <QApplication>
#include <infodialog.h>
#include <KLocale>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "gidmigrator.h"

int main(int argc, char **argv)
{
    KAboutData aboutData(QStringLiteral("gid-migrator"),
                            i18n("GID Migration Tool"),
                            QStringLiteral("0.1"),
                            i18n("Migration of Akonadi Items to support GID"),
                            KAboutLicense::LGPL,
                            i18n("(c) 2013 the Akonadi developers"),
                            QStringLiteral("http://pim.kde.org/akonadi/"));
    aboutData.setProgramIconName(QLatin1String("akonadi"));
    aboutData.addAuthor(i18n("Christian Mollekopf"),  i18n("Author"), QStringLiteral("mollekopf@kolabsys.com"));

    QCommandLineParser parser;
    QApplication app(argc, argv);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("interactive"), i18n("Show reporting dialog")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("interactive-on-change"), i18n("Show report only if changes were made")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("mimetype"), i18n("Mimetype to migrate")));

    app.setQuitOnLastWindowClosed(false);

    KGlobal::setAllowQuit(true);

    if (!Akonadi::Control::start(0)) {
        return 2;
    }

    InfoDialog *infoDialog = 0;
    if (parser.isSet(QLatin1String("interactive")) || parser.isSet(QLatin1String("interactive-on-change"))) {
        infoDialog = new InfoDialog(parser.isSet(QLatin1String("interactive-on-change")));
        Akonadi::Control::widgetNeedsAkonadi(infoDialog);
        infoDialog->show();
    }

    const QString mimeType = parser.value(QLatin1String("mimetype"));
    if (mimeType.isEmpty()) {
        qWarning() << "set the mimetype to migrate";
        return 5;
    }

    GidMigrator *migrator = new GidMigrator(mimeType);
    if (infoDialog && migrator) {
        infoDialog->migratorAdded();
        QObject::connect(migrator, SIGNAL(message(MigratorBase::MessageType,QString)),
                        infoDialog, SLOT(message(MigratorBase::MessageType,QString)));
        QObject::connect(migrator, SIGNAL(destroyed()), infoDialog, SLOT(migratorDone()));
        QObject::connect(migrator, SIGNAL(progress(int)), infoDialog, SLOT(progress(int)));
    }
    QObject::connect(migrator, SIGNAL(stoppedProcessing()), &app, SLOT(quit));
    migrator->start();
    const int result = app.exec();
    if (InfoDialog::hasError() || migrator->migrationState() == MigratorBase::Failed) {
        return 3;
    }

    return result;
}
