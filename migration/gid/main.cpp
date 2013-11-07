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

#include <Akonadi/Control>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <KApplication>
#include <infodialog.h>

#include "gidmigrator.h"

int main(int argc, char **argv)
{
    KAboutData aboutData("gid-migrator", 0,
                            ki18n("GID Migration Tool"),
                            "0.1",
                            ki18n("Migration of Akonadi Items to support GID"),
                            KAboutData::License_LGPL,
                            ki18n("(c) 2013 the Akonadi developers"),
                            KLocalizedString(),
                            "http://pim.kde.org/akonadi/");
    aboutData.setProgramIconName(QLatin1String("akonadi"));
    aboutData.addAuthor(ki18n("Christian Mollekopf"),  ki18n("Author"), "mollekopf@kolabsys.com");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineOptions options;
    options.add("interactive", ki18n("Show reporting dialog"));
    options.add("interactive-on-change", ki18n("Show report only if changes were made"));
    options.add("mimetype", ki18n("Mimetype to migrate"));
    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication *app = new KApplication();
    app->setQuitOnLastWindowClosed(false);

    KGlobal::setAllowQuit(true);
    KGlobal::locale()->insertCatalog(QLatin1String("libakonadi"));

    if (!Akonadi::Control::start(0)) {
        return 2;
    }

    InfoDialog *infoDialog = 0;
    if (args->isSet("interactive") || args->isSet("interactive-on-change")) {
        infoDialog = new InfoDialog(args->isSet("interactive-on-change"));
        Akonadi::Control::widgetNeedsAkonadi(infoDialog);
        infoDialog->show();
    }

    const QString mimeType = args->getOption("mimetype");
    if (mimeType.isEmpty()) {
        kWarning() << "set the mimetype to migrate";
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
    QObject::connect(migrator, SIGNAL(stoppedProcessing()), app, SLOT(quit));
    migrator->start();
    const int result = app->exec();
    if (InfoDialog::hasError() || migrator->migrationState() == MigratorBase::Failed) {
        return 3;
    }

    return result;
}
