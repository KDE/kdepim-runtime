/* This file is part of the KDE project
   Copyright (C) 2006-2008 Omat Holding B.V. <info@omat.nl>
   Copyright (C) 2006 Frode M. Døving <frode@lnix.net>

   Original copied from showfoto:
    Copyright 2005 by Gilles Caulier <caulier.gilles@free.fr>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "setupserver.h"
#include "settings.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QRadioButton>

#include <mailtransport/transport.h>
#include <mailtransport/servertest.h>

#include <kemailsettings.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kmessagebox.h>
#include <kuser.h>
#include <solid/networking.h>

SetupServer::SetupServer( QWidget* parent )
        : KDialog( parent )
{
    QGridLayout *mainGrid = new QGridLayout( mainWidget() );

    QLabel *l4 = new QLabel( i18n( "IMAP server:" ) + ' ', mainWidget() );
    mainGrid->addWidget( l4, 0, 0 );
    l4->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    l4->setWhatsThis( i18n( "Indicate the IMAP server. If you want to "
                            "connect to a non-standard port for a chosen safety, you can add "
                            "\":port\" to indicate that. For example: \"imap.bla.nl:144\"" ) );

    m_imapServer = new KLineEdit( mainWidget() );
    mainGrid->addWidget( m_imapServer, 0, 1 );
    l4->setBuddy( m_imapServer );
    connect( m_imapServer, SIGNAL( textChanged( const QString & ) ),
             SLOT( slotTestChanged() ) );
    connect( m_imapServer, SIGNAL( textChanged( const QString & ) ),
             SLOT( slotComplete() ) );

    QLabel *l5 = new QLabel( i18n( "Safety:" ) + ' ', mainWidget() );
    mainGrid->addWidget( l5, 2, 0 );
    l5->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    l5->setWhatsThis( i18n( "<b>SSL</b> is safe IMAP over port 993, <b>TLS</b> "
                            "will operate on port 143 and switch to a secure connection "
                            "directly after connecting and <b>None</b> will connect to port "
                            "143 but not switch to a secure connection. This setting is not "
                            "recommended." ) );

    m_safeImap = new QGroupBox( mainWidget() );
    QHBoxLayout* safePartLay = new QHBoxLayout( m_safeImap );
    safePartLay->setMargin( 0 );
    safePartLay->setSpacing( KDialog::spacingHint() );
    m_testButton = new KPushButton( i18n( "Auto detect" ), m_safeImap );
    safePartLay->addWidget( m_testButton );
    m_noRadio = new QRadioButton( i18n( "None" ), m_safeImap );
    safePartLay->addWidget( m_noRadio );
    m_sslRadio = new QRadioButton( i18n( "SSL" ), m_safeImap );
    safePartLay->addWidget( m_sslRadio );
    m_tlsRadio = new QRadioButton( i18n( "TLS" ), m_safeImap );
    safePartLay->addWidget( m_tlsRadio );
    mainGrid->addWidget( m_safeImap,2,1 );
    l5->setBuddy( m_safeImap );

    connect( m_testButton, SIGNAL( pressed() ), SLOT( slotTest() ) );

    m_safeImap_group = new QButtonGroup( mainWidget() );
    m_safeImap_group->addButton( m_noRadio,1 );
    m_safeImap_group->addButton( m_sslRadio,2 );
    m_safeImap_group->addButton( m_tlsRadio,3 );

    QLabel *l6 = new QLabel( i18n( "Username:" ) + ' ', mainWidget() );
    mainGrid->addWidget( l6,3,0 );
    l6->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    l6->setWhatsThis( i18n( "The username" ) );
    m_userName = new KLineEdit( mainWidget() );
    mainGrid->addWidget( m_userName,3,1 );
    l6->setBuddy( m_userName );
    connect( m_userName, SIGNAL( textChanged( const QString & ) ),
             SLOT( slotComplete() ) );

    QLabel *l7 = new QLabel( i18n( "Password:" ) + ' ', mainWidget() );
    mainGrid->addWidget( l7,4,0 );
    l7->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    l7->setWhatsThis( i18n( "The password" ) );
    m_password = new KLineEdit( mainWidget() );
    m_password->setPasswordMode( true );
    mainGrid->addWidget( m_password,4, 1 );
    l7->setBuddy( m_password );

    QSpacerItem* spacer2 = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                                            QSizePolicy::Expanding );
    mainGrid->addItem( spacer2, 5, 1 );

    m_testInfo = new QLabel( mainWidget() );
    m_testInfo->setAlignment( Qt::AlignHCenter );
    m_testInfo->hide();
    mainGrid->addWidget( m_testInfo, 6, 1 );

    m_testProgress = new QProgressBar( mainWidget() );
    mainGrid->addWidget( m_testProgress, 7, 1 );
    m_testProgress->hide();

    QSpacerItem* spacer3 = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                                            QSizePolicy::Expanding );
    mainGrid->addItem( spacer3, 8, 1 );

    readSettings();
    slotTestChanged();
    slotComplete();
    connect( Solid::Networking::notifier(),
             SIGNAL( statusChanged( Solid::Networking::Status ) ),
             SLOT( slotTestChanged() ) );
    connect( this, SIGNAL( applyClicked() ),
             SLOT( applySettings() ) );
    connect( this, SIGNAL( okClicked() ),
             SLOT( applySettings() ) );
}

SetupServer::~SetupServer()
{
}

void SetupServer::applySettings()
{
    Settings::self()->setImapServer( m_imapServer->text() );
    Settings::self()->setUsername( m_userName->text() );
    Settings::self()->setSafety( m_safeImap_group->checkedId() );
    Settings::self()->setPassword( m_password->text() );
    Settings::self()->writeConfig();
    kDebug() << "wrote" << m_imapServer->text() << m_userName->text() << m_safeImap_group->checkedId();
}

void SetupServer::readSettings()
{
    KUser* currentUser = new KUser();
    KEMailSettings esetting;

    m_imapServer->setText(
        !Settings::self()->imapServer().isEmpty() ? Settings::self()->imapServer() :
        esetting.getSetting( KEMailSettings::InServer ) );
    m_userName->setText(
        !Settings::self()->username().isEmpty() ? Settings::self()->username() :
        currentUser->loginName() );
    int i = Settings::self()->safety();
    if ( i == 0 )
        i = 1; // it crashes when 0, shouldn't happen, but you never know.
    m_safeImap_group->button( i )->setChecked( true );

    if ( !Settings::self()->passwordPossible() ) {
        m_password->setEnabled( false );
        KMessageBox::information( 0,i18n( "Could not access KWallet, "
                                          "if you want to store the password permanently then you have to "
                                          "activate it. If you do not "
                                          "want to use KWallet, check the box below and enjoy the dialogs." ),
                                  i18n( "Do not use KWallet" ), "warning_kwallet_disabled" );
    } else
        m_password->insert( Settings::self()->password() );
}

void SetupServer::slotTest()
{
    kDebug() << m_imapServer->text() << endl;

    m_testInfo->clear();
    m_sslRadio->setEnabled( false );
    m_tlsRadio->setEnabled( false );
    m_noRadio->setEnabled( false );


    MailTransport::ServerTest* test = new MailTransport::ServerTest( this );
    test->setServer( m_imapServer->text() );
    test->setProtocol( "imap" );
    test->setProgressBar( m_testProgress );
    connect( test, SIGNAL( finished( QList<int> ) ),
             SLOT( slotFinished( QList<int> ) ) );
    test->start();
}

void SetupServer::slotFinished( QList<int> testResult )
{
    kDebug() << testResult << endl;

    using namespace MailTransport;

    m_testInfo->show();

    if ( testResult.contains( Transport::EnumEncryption::SSL ) )
        m_sslRadio->setEnabled( true );

    if ( testResult.contains( Transport::EnumEncryption::TLS ) )
        m_tlsRadio->setEnabled( true );

    if ( testResult.contains( Transport::EnumEncryption::None ) )
        m_noRadio->setEnabled( true );

    QString text;
    if ( testResult.contains( Transport::EnumEncryption::SSL ) ) {
        m_safeImap_group->button( 2 )->setChecked( true );
        text = i18n( "<qt><b>SSL is supported and recommended</b></qt>" );
    } else if ( testResult.contains( Transport::EnumEncryption::TLS ) ) {
        m_safeImap_group->button( 3 )->setChecked( true );
        text = i18n( "<qt><b>TLS is supported and recommended</b></qt>" );
    } else if ( testResult.contains( Transport::EnumEncryption::None ) ) {
        m_safeImap_group->button( 1 )->setChecked( true );
        text = i18n( "<qt><b>No security is supported. It is not "
                     "recommended to connect to this server.</b></qt>" );
    } else {
        text = i18n( "<qt><b>It is not possible to use this server.</b></qt>" );
    }
    m_testInfo->setText( text );
}

void SetupServer::slotTestChanged()
{
    // dont use imapConnectionPossible, as the data is not yet saved.
    m_testButton->setEnabled( true /* TODO Global::connectionPossible() ||
                              m_imapServer->text() == "localhost"*/ );
}

void SetupServer::slotComplete()
{
   bool ok =  !m_imapServer->text().isEmpty() && !m_userName->text().isEmpty();
   button( KDialog::Ok )->setEnabled( ok );
}

#include "setupserver.moc"
