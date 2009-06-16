/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "configdialog.h"
#include "settings.h"

#include <kconfigdialogmanager.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <KUser>

#include <QWhatsThis>

ConfigDialog::ConfigDialog( QWidget * parent ) :
        KDialog( parent )
{
    m_comm = new Communication( this );
    connect( m_comm, SIGNAL( authOk() ), SLOT( slotAuthOk() ) );
    connect( m_comm, SIGNAL( authFailed( const QString& ) ), SLOT( slotAuthFailed( const QString& ) ) );
    ui.setupUi( mainWidget() );
    mManager = new KConfigDialogManager( this, Settings::self() );
    mManager->updateWidgets();
    ui.password->setText( Settings::self()->password() );
    if (ui.kcfg_Name->text().isEmpty() ) {
        KUser user;
        QString usersName = user.property( KUser::FullName ).toString();
        if ( usersName.isEmpty() )
            usersName = user.loginName();
        if ( !usersName.isEmpty() )
            ui.kcfg_Name->setText( usersName );
    }
    setButtons( KDialog::Ok | KDialog::Cancel );
    ui.testButton->setEnabled(!ui.kcfg_Name->text().isEmpty());
    connect( ui.testButton, SIGNAL( clicked() ), SLOT( slotTestClicked() ) );
    connect( ui.kcfg_UserName, SIGNAL(textChanged(const QString&)), SLOT(slotTextChanged(const QString&)));
    connect( ui.openidLabel, SIGNAL( linkActivated ( const QString & ) ), SLOT( slotLinkClicked() ) );
}

ConfigDialog::~ConfigDialog()
{
    delete m_comm;
}

void ConfigDialog::slotTestClicked()
{
    kDebug() << "Test request" << ui.kcfg_Service->currentIndex()
    << ui.kcfg_UserName->text() << ui.password->text();
    setCursor( Qt::BusyCursor );
    ui.statusLabel->setText( i18n( "Checking..." ) );
    ui.testButton->setEnabled( false );
    m_comm->setService( ui.kcfg_Service->currentIndex() );
    m_comm->setCredentials( ui.kcfg_UserName->text(), ui.password->text() );
    m_comm->checkAuth();
}

void ConfigDialog::slotTextChanged(const QString &text)
{
    ui.testButton->setEnabled(!text.isEmpty());
}

void ConfigDialog::slotButtonClicked( int button )
{
    if (button == KDialog::Ok) {
        Settings::self()->setPassword( ui.password->text() );
        mManager->updateSettings();
        accept();
    }  else {
        KDialog::slotButtonClicked(button);
    }
 }

void ConfigDialog::slotAuthOk()
{
    unsetCursor();
    ui.testButton->setEnabled( true );
    ui.statusLabel->setText( i18n( "OK" ) );
    ui.statusImageLabel->setPixmap( KIcon( "dialog-ok" ).pixmap( 16 ) );
    Settings::self()->setPassword( ui.password->text() );
    mManager->updateSettings();
}

void ConfigDialog::slotAuthFailed( const QString& error )
{
    unsetCursor();
    ui.statusLabel->setText( i18n( "Failed" ) );
    ui.statusImageLabel->setPixmap( KIcon( "dialog-cancel" ).pixmap( 16 ) );
    ui.testButton->setEnabled( true );
}

void ConfigDialog::slotLinkClicked()
{
    QWhatsThis::showText( QCursor::pos(), i18n( "OpenId users must first specify a password in "
        "the settings on the webpage, as this resource cannot use OpenId." ), this );
}

#include "configdialog.moc"
