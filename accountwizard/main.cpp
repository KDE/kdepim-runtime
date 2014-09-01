/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "dialog.h"
#include "global.h"

#include <control.h>

#include <kaboutdata.h>
#include <qapplication.h>
#include <QUrl>

#include <KDBusService>
#include <KLocalizedString>
#include <stdio.h>
#include <QCommandLineParser>
#include <QCommandLineOption>

int main(int argc, char **argv)
{
    KLocalizedString::setApplicationDomain("accountwizard");
    KAboutData aboutData(QLatin1String("accountwizard"),
                         i18n("Account Assistant"),
                         QLatin1String("0.1"),
                         i18n("Helps setting up PIM accounts"),
                         KAboutLicense::LGPL,
                         i18n("(c) 2009 the Akonadi developers"),
                         QLatin1String("http://pim.kde.org/akonadi/"));
    aboutData.setProgramIconName(QLatin1String("akonadi"));
    aboutData.addAuthor(i18n("Volker Krause"),  i18n("Author"), QLatin1String("vkrause@kde.org"));
    aboutData.addAuthor(i18n("Laurent Montel"), QString() , QLatin1String("montel@kde.org"));

    QApplication app(argc, argv);
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("type"), i18n("Only offer accounts that support the given type."), QLatin1String("type")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("assistant"), i18n("Run the specified assistant."), QLatin1String("assistant")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("package"), i18n("unpack fullpath on startup and launch that assistant"), QLatin1String("fullpath")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    KDBusService service(KDBusService::Unique);

    Akonadi::Control::start(0);

    if (!parser.value(QLatin1String("package")).isEmpty()) {
        Global::setAssistant(Global::unpackAssistant(QUrl::fromLocalFile(parser.value(QLatin1String("package")))));
    } else {
        Global::setAssistant(parser.value(QLatin1String("assistant")));
    }

    if (!parser.value(QLatin1String("type")).isEmpty()) {
        Global::setTypeFilter(parser.value(QLatin1String("type")).split(QLatin1Char(',')));
    }

    Dialog dlg(0/*, Qt::WindowStaysOnTopHint*/);
    dlg.show();

    return app.exec();
}
