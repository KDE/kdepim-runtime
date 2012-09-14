/*
    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include "nepomukpimindexerutility.h"
#include <KDE/KApplication>
#include <KDE/KAboutData>
#include <KDE/KCmdLineArgs>
#include <KDE/KLocale>

static const char description[] =
    I18N_NOOP("A Developer Utility to check which PIM Items have been indexed.");

static const char version[] = "0.1";

int main(int argc, char **argv)
{
    KAboutData about("nepomukpimindexerutility", 0, ki18n("NepomukPIMindexerUtility"), version, ki18n(description),
                     KAboutData::License_LGPL, ki18n("(C) 2012 Christian Mollekopf"), KLocalizedString(), 0, "chrigi_1@fastmail.fm");
    about.addAuthor( ki18n("Christian Mollekopf"), KLocalizedString(), "chrigi_1@fastmail.fm" );
    KCmdLineArgs::init(argc, argv, &about);

    KApplication app;

    NepomukPIMindexerUtility *mainWindow = new NepomukPIMindexerUtility;
    mainWindow->show();

    return app.exec();
}

