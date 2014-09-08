/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Tom Albers <toma@kde.org>
    Copyright (c) 2012 Laurent Montel <montel@kde.org>

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

#include "personaldatapage.h"

#include "config-enterprise.h"
#include "global.h"
#include "dialog.h"
#include "transport.h"
#include "resource.h"
#include "ispdb/ispdb.h"

#include <kpimutils/emailvalidator.h>
#include <kpimutils/email.h>

#include <mailtransport/transport.h>

#include <QDebug>

QString accountName(Ispdb *ispdb, QString username)
{
    const int pos(username.indexOf(QLatin1Char('@')));
    username = username.left(pos);
    return ispdb->name(Ispdb::Long) + QString::fromLatin1(" (%1)").arg(username);
}

PersonalDataPage::PersonalDataPage(Dialog *parent) :
    Page(parent), mIspdb(0), mSetupManager(parent->setupManager())
{
    QWidget *pageParent = this;

    ui.setupUi(pageParent);

    KPIMUtils::EmailValidator *emailValidator = new KPIMUtils::EmailValidator(this);
    ui.emailEdit->setValidator(emailValidator);

    // KEmailSettings defaults
    ui.nameEdit->setText(mSetupManager->name());
    ui.emailEdit->setText(mSetupManager->email());
    slotTextChanged();
    connect(ui.emailEdit, &QLineEdit::textChanged, this, &PersonalDataPage::slotTextChanged);
    connect(ui.nameEdit, &QLineEdit::textChanged, this, &PersonalDataPage::slotTextChanged);
    connect(ui.createAccountPb, &QPushButton::clicked, this, &PersonalDataPage::slotCreateAccountClicked);
    connect(ui.buttonGroup, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked), this, &PersonalDataPage::slotRadioButtonClicked);
#ifdef KDEPIM_ENTERPRISE_BUILD
    ui.checkOnlineGroupBox->setChecked(false);
#endif
}

void PersonalDataPage::setHideOptionInternetSearch(bool hide)
{
    ui.checkOnlineGroupBox->setChecked(!hide);
    ui.checkOnlineGroupBox->setVisible(!hide);
}

void PersonalDataPage::slotRadioButtonClicked(QAbstractButton *button)
{
    QString smptHostname;
    if (!mIspdb->smtpServers().isEmpty()) {
        server s = mIspdb->smtpServers().first();
        smptHostname = s.hostname;
    }
    ui.outgoingLabel->setText(i18n("SMTP, %1", smptHostname));
    if (button ==  ui.imapAccount) {
        server simap = mIspdb->imapServers().first(); // should be ok.
        ui.incommingLabel->setText(i18n("IMAP, %1", simap.hostname));
        ui.usernameLabel->setText(simap.username);
    } else if (button == ui.pop3Account) {
        server spop3 = mIspdb->pop3Servers().first(); // should be ok.
        ui.incommingLabel->setText(i18n("POP3, %1", spop3.hostname));
        ui.usernameLabel->setText(spop3.username);
    }
}

void PersonalDataPage::slotCreateAccountClicked()
{
    configureSmtpAccount();
    if (ui.imapAccount->isChecked()) {
        configureImapAccount();
    } else {
        configurePop3Account();
    }
    emit leavePageNextOk();  // go to the next page
    mSetupManager->execute();
}

void PersonalDataPage::slotTextChanged()
{
    // Ignore the password field, as that can be empty when auth is based on ip-address.
    setValid(!ui.emailEdit->text().isEmpty() &&
             !ui.nameEdit->text().isEmpty()  &&
             KPIMUtils::isValidSimpleAddress(ui.emailEdit->text()));
}

void PersonalDataPage::leavePageNext()
{
    ui.stackedPage->setCurrentIndex(0);
    ui.imapAccount->setChecked(true);
    mSetupManager->setPersonalDataAvailable(true);
    mSetupManager->setName(ui.nameEdit->text());
    mSetupManager->setPassword(ui.passwordEdit->text());
    mSetupManager->setEmail(ui.emailEdit->text().trimmed());

    if (ui.checkOnlineGroupBox->isChecked()) {
        // since the user can go back and forth, explicitly disable the man page
        emit manualWanted(false);
        setCursor(Qt::BusyCursor);
        ui.mProgress->start();
        qDebug() << "Searching on internet";
        delete mIspdb;
        mIspdb = new Ispdb(this);
        connect(mIspdb, &Ispdb::searchType, this, &PersonalDataPage::slotSearchType);
        mIspdb->setEmail(ui.emailEdit->text());
        mIspdb->start();

        connect(mIspdb, &Ispdb::finished, this, &PersonalDataPage::ispdbSearchFinished);
    } else {
        emit manualWanted(true);       // enable the manual page
        emit leavePageNextOk();  // go to the next page
    }
}

void PersonalDataPage::ispdbSearchFinished(bool ok)
{
    qDebug() << ok;

    unsetCursor();
    ui.mProgress->stop();
    if (ok) {

        if (!mIspdb->imapServers().isEmpty() && !mIspdb->pop3Servers().isEmpty()) {
            ui.stackedPage->setCurrentIndex(1);
            slotRadioButtonClicked(ui.imapAccount);
        } else {
            automaticConfigureAccount();
        }

    } else {
        emit manualWanted(true);       // enable the manual page
        emit leavePageNextOk();
    }
}

void PersonalDataPage::slotSearchType(const QString &type)
{
    ui.mProgress->setActiveLabel(type);
}

void PersonalDataPage::configureSmtpAccount()
{
    if (!mIspdb->smtpServers().isEmpty()) {
        server s = mIspdb->smtpServers().first(); // should be ok.
        qDebug() << "Configuring transport for" << s.hostname;

        QObject *object = mSetupManager->createTransport(QLatin1String("smtp"));
        Transport *t = qobject_cast<Transport *>(object);
        t->setName(accountName(mIspdb, s.username));
        t->setHost(s.hostname);
        t->setPort(s.port);
        t->setUsername(s.username);
        t->setPassword(ui.passwordEdit->text());
        switch (s.authentication) {
        case Ispdb::Plain: t->setAuthenticationType(QLatin1String("plain")); break;
        case Ispdb::CramMD5: t->setAuthenticationType(QLatin1String("cram-md5")); break;
        case Ispdb::NTLM: t->setAuthenticationType(QLatin1String("ntlm")); break;
        case Ispdb::GSSAPI: t->setAuthenticationType(QLatin1String("gssapi")); break;
        case Ispdb::ClientIP: break;
        case Ispdb::NoAuth: break;
        default: break;
        }
        switch (s.socketType) {
        case Ispdb::Plain: t->setEncryption(QLatin1String("none")); break;
        case Ispdb::SSL: t->setEncryption(QLatin1String("ssl")); break;
        case Ispdb::StartTLS: t->setEncryption(QLatin1String("tls")); break;
        default: break;
        }
    } else {
        qDebug() << "No transport to be created....";
    }
}

void PersonalDataPage::configureImapAccount()
{
    if (!mIspdb->imapServers().isEmpty()) {
        server s = mIspdb->imapServers().first(); // should be ok.
        qDebug() << "Configuring imap for" << s.hostname;

        QObject *object = mSetupManager->createResource(QLatin1String("akonadi_imap_resource"));
        Resource *t = qobject_cast<Resource *>(object);
        t->setName(accountName(mIspdb, s.username));
        t->setOption(QLatin1String("ImapServer"), s.hostname);
        t->setOption(QLatin1String("ImapPort"), s.port);
        t->setOption(QLatin1String("UserName"), s.username);
        t->setOption(QLatin1String("Password"), ui.passwordEdit->text());
        switch (s.authentication) {
        case Ispdb::Plain: t->setOption(QLatin1String("Authentication"), MailTransport::Transport::EnumAuthenticationType::CLEAR); break;
        case Ispdb::CramMD5: t->setOption(QLatin1String("Authentication"), MailTransport::Transport::EnumAuthenticationType::CRAM_MD5); break;
        case Ispdb::NTLM: t->setOption(QLatin1String("Authentication"), MailTransport::Transport::EnumAuthenticationType::NTLM); break;
        case Ispdb::GSSAPI: t->setOption(QLatin1String("Authentication"), MailTransport::Transport::EnumAuthenticationType::GSSAPI); break;
        case Ispdb::ClientIP: break;
        case Ispdb::NoAuth: break;
        default: break;
        }
        switch (s.socketType) {
        case Ispdb::None: t->setOption(QLatin1String("Safety"), QLatin1String("None")); break;
        case Ispdb::SSL: t->setOption(QLatin1String("Safety"), QLatin1String("SSL")); break;
        case Ispdb::StartTLS: t->setOption(QLatin1String("Safety"), QLatin1String("STARTTLS")); break;
        default: break;
        }
    }
}

void PersonalDataPage::configurePop3Account()
{
    if (!mIspdb->pop3Servers().isEmpty()) {
        server s = mIspdb->pop3Servers().first(); // should be ok.
        qDebug() << "No Imap to be created, configuring pop3 for" << s.hostname;

        QObject *object = mSetupManager->createResource(QLatin1String("akonadi_pop3_resource"));
        Resource *t = qobject_cast<Resource *>(object);
        t->setName(accountName(mIspdb, s.username));
        t->setOption(QLatin1String("Host"), s.hostname);
        t->setOption(QLatin1String("Port"), s.port);
        t->setOption(QLatin1String("Login"), s.username);
        t->setOption(QLatin1String("Password"), ui.passwordEdit->text());
        switch (s.authentication) {
        case Ispdb::Plain: t->setOption(QLatin1String("AuthenticationMethod"), MailTransport::Transport::EnumAuthenticationType::PLAIN); break;
        case Ispdb::CramMD5: t->setOption(QLatin1String("AuthenticationMethod"), MailTransport::Transport::EnumAuthenticationType::CRAM_MD5); break;
        case Ispdb::NTLM: t->setOption(QLatin1String("AuthenticationMethod"), MailTransport::Transport::EnumAuthenticationType::NTLM); break;
        case Ispdb::GSSAPI: t->setOption(QLatin1String("AuthenticationMethod"), MailTransport::Transport::EnumAuthenticationType::GSSAPI); break;
        case Ispdb::ClientIP:
        case Ispdb::NoAuth:
        default: t->setOption(QLatin1String("AuthenticationMethod"), MailTransport::Transport::EnumAuthenticationType::CLEAR); break;
        }
        switch (s.socketType) {
        case Ispdb::SSL: t->setOption(QLatin1String("UseSSL"), 1); break;
        case Ispdb::StartTLS: t->setOption(QLatin1String("UseTLS"), 1); break;
        case Ispdb::None:
        default: t->setOption(QLatin1String("UseTLS"), 1); break;
        }
    }
}

void PersonalDataPage::automaticConfigureAccount()
{
    configureSmtpAccount();
    configureImapAccount();
    configurePop3Account();
    emit leavePageNextOk();  // go to the next page
    mSetupManager->execute();
}

void PersonalDataPage::leavePageNextRequested()
{
    // override base class with doing nothing...
}

