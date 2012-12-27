/*
    Copyright (c) 2012 Frank Osterfeld <osterfeld@kde.org>

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

#include <KConfigDialogManager>
#include <KWallet/Wallet>

#include <QPointer>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace KWallet;

const char * const folderName = "Akonadi Owncloud";

TestLoginDialog::TestLoginDialog( QWidget* parent )
    : KDialog( parent )
    , m_textBrowser( new QTextBrowser )
{
    QVBoxLayout* layout = new QVBoxLayout( mainWidget() );
    layout->addWidget( m_textBrowser );
    setButtons( KDialog::Ok|KDialog::Cancel );
}

void TestLoginDialog::startTest( const KUrl& url, const QString& password )
{
}

ConfigDialog::ConfigDialog( const QString& resourceId, QWidget *parent )
    : KDialog( parent )
    , m_manager( new KConfigDialogManager( this, Settings::self() ) )
    , m_wallet( Wallet::openWallet( QString(), winId(), Wallet::Asynchronous ) )
    , m_resourceId( resourceId )
{
    Q_ASSERT( m_wallet );
    connect( m_wallet, SIGNAL(walletOpened(bool)), this, SLOT(walletOpened(bool)) );
    ui.setupUi( mainWidget() );
    m_manager->updateWidgets();
    connect( ui.testButton, SIGNAL(clicked()), this, SLOT(testLogin()) );
    ui.owncloudServerLineEdit->setText( Settings::self()->owncloudServerUrl() );
}

ConfigDialog::~ConfigDialog()
{
    delete m_wallet;
}

void ConfigDialog::walletOpened( bool success )
{
    if ( !success ) {
        //TODO show error
        return;
    }

    if ( !m_wallet->hasFolder( QLatin1String(folderName) ) )
        return;

    m_wallet->setFolder( QLatin1String(folderName) );
    QString password;
    const int ret = m_wallet->readPassword( m_resourceId, /*out*/ password );
    if ( ret == 0 ) {
        ui.passwordLineEdit->setText( password );
    } else {
        //TODO handle error
    }
}

void ConfigDialog::save()
{
    m_manager->updateSettings();
    if ( !m_wallet->isOpen() )
        return;

    if ( !m_wallet->hasFolder( QLatin1String(folderName) ) ) {
        const bool created = m_wallet->createFolder( QLatin1String(folderName) ); //TODO possible race condition?
        if ( !created ) {
            //TODO handle error
            return;
        }
    }
    if ( !m_wallet->setFolder( QLatin1String(folderName) ) ) {
        //TODO handle error
        return;
    }
    const int ret = m_wallet->writePassword( m_resourceId, ui.passwordLineEdit->text() );
    if ( ret != 0 ) {
        //TODO show error
    }
}

void ConfigDialog::testLogin()
{
    QPointer<TestLoginDialog> testLoginDialog( new TestLoginDialog( this ) );
    testLoginDialog->setAttribute( Qt::WA_DeleteOnClose );
    testLoginDialog->startTest( ui.owncloudServerLineEdit->text(), ui.passwordLineEdit->text()  );
    testLoginDialog->exec();
}

#include "configdialog.moc"
