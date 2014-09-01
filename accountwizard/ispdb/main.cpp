/*
    Copyright (c) 2010 Omat Holding B.V. <info@omat.nl>

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

#include "ispdb.h"

#include <kaboutdata.h>

#include <KLocalizedString>
#include <QDebug>
#include <QApplication>
#include <KAboutData>
#include <QCommandLineParser>
#include <QCommandLineOption>

QString socketTypeToStr(Ispdb::socketType socketType)
{
    QString enc = QLatin1String("None");
    if (socketType == Ispdb::SSL) {
        enc = QLatin1String("SSL");
    } else if (socketType == Ispdb::StartTLS) {
        enc = QLatin1String("TLS");
    }
    return enc;
}

QString authTypeToStr(Ispdb::authType authType)
{
    QString auth = QLatin1String("unknown");
    switch (authType) {
    case Ispdb::Plain:
        auth = QLatin1String("plain");
        break;
    case Ispdb::CramMD5:
        auth = QLatin1String("CramMD5");
        break;
    case Ispdb::NTLM:
        auth = QLatin1String("NTLM");
        break;
    case Ispdb::GSSAPI:
        auth = QLatin1String("GSSAPI");
        break;
    case Ispdb::ClientIP:
        auth = QLatin1String("ClientIP");
        break;
    case Ispdb::NoAuth:
        auth = QLatin1String("NoAuth");
        break;
    }
    return auth;
}
int main(int argc, char **argv)
{
    KAboutData aboutData(QLatin1String("ispdb"),
                         i18n("ISPDB Assistant"),
                         QLatin1String("0.1"),
                         i18n("Helps setting up PIM accounts"),
                         KAboutLicense::LGPL,
                         i18n("(c) 2010 Omat Holding B.V."),
                         QLatin1String("http://pim.kde.org/akonadi/"));
    aboutData.setProgramIconName(QLatin1String("akonadi"));
    aboutData.addAuthor(i18n("Tom Albers"),  i18n("Author"), QLatin1String("toma@kde.org"));

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("email"), i18n("Tries to fetch the settings for that email address"), QLatin1String("emailaddress")));

    QString email(QLatin1String("blablabla@gmail.com"));
    if (!parser.value(QLatin1String("email")).isEmpty()) {
        email = parser.value(QLatin1String("email"));
    }

    QEventLoop loop;
    Ispdb ispdb;
    ispdb.setEmail(email);

    loop.connect(&ispdb, SIGNAL(finished(bool)), SLOT(quit()));

    ispdb.start();

    loop.exec();
    qDebug() << "Domains" << ispdb.relevantDomains();
    qDebug() << "Name" << ispdb.name(Ispdb::Long) << "(" << ispdb.name(Ispdb::Short) << ")";
    qDebug() << "Imap servers:";
    foreach (const server &s, ispdb.imapServers()) {
        qDebug() << "\thostname:" << s.hostname
                 << "- port:" << s.port
                 << "- encryption:" << socketTypeToStr(s.socketType)
                 << "- username:" << s.username
                 << "- authentication:" << authTypeToStr(s.authentication);
    }
    qDebug() << "pop3 servers:";
    foreach (const server &s, ispdb.pop3Servers()) {
        qDebug() << "\thostname:" << s.hostname
                 << "- port:" << s.port
                 << "- encryption:" << socketTypeToStr(s.socketType)
                 << "- username:" << s.username
                 << "- authentication:" << authTypeToStr(s.authentication);
    }
    qDebug() << "smtp servers:";
    foreach (const server &s, ispdb.smtpServers()) {
        qDebug() << "\thostname:" << s.hostname
                 << "- port:" << s.port
                 << "- encryption:" << socketTypeToStr(s.socketType)
                 << "- username:" << s.username
                 << "- authentication:" << authTypeToStr(s.authentication);
    }

    return true;
}
