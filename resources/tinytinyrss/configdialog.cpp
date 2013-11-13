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
#include "jobs.h"
#include "settings.h"

#include <KWallet/Wallet>
#include <KMessageBox>

#include <QPointer>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace KWallet;

TestLoginDialog::TestLoginDialog( QWidget* parent )
    : KDialog( parent )
    , m_textBrowser( new QTextBrowser )
{
    QVBoxLayout* layout = new QVBoxLayout( mainWidget() );
    layout->addWidget( m_textBrowser );
    setButtons( KDialog::Ok|KDialog::Cancel );
}

void TestLoginDialog::startTest( const KUrl& url, const QString& username, const QString& password )
{
    TransferJob* job = new TransferJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(loginFinished(KJob*)) );
    job->setUrl( url );
    job->setOperation( QLatin1String("login") );
    job->insertData( QLatin1String("user"), username );
    job->insertData( QLatin1String("password"), password );
    job->start();
}

static bool hasError( TransferJob* job, QString* errorString ) {
    if ( job->error() != KJob::NoError ) { //TODO handle different codes differently? (e.g. network errors)
        *errorString = i18n("Login failed: %1", job->errorString());
        return true;
    }
    const QVariantMap map = job->responseContent().toMap();

    const QString sessionId = map.value( QLatin1String("session_id") ).toString();
    if ( sessionId.isEmpty() ) {
        *errorString = i18n("Empty session ID");
        return true;
    }

    bool ok;
    const int apiLevel = map.value( QLatin1String("api_level") ).toInt( &ok );
    if ( !ok ) {
        *errorString = i18n("Could not parse API Level (%1)", map.value( QLatin1String("api_level") ).toString() );
        return true;
    }
    if ( apiLevel != 4 ) {
        *errorString = i18n("Unsupported API level %1 (supported: 4)", QString::number( apiLevel ) );
        return true;
    }

    return false;
}

void TestLoginDialog::loginFinished( KJob* j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    Q_ASSERT( job );

    QString errorMessage;
    if ( hasError( job, &errorMessage ) ) {
        m_textBrowser->append( i18n("Error: %1", errorMessage) );
    } else {
        m_textBrowser->append( i18n("Test successful") );
    }
}

ConfigDialog::ConfigDialog( const QString& resourceId, QWidget *parent )
    : KDialog( parent )
    , m_resourceId( resourceId )
{
    ui.setupUi( mainWidget() );
    connect( ui.testButton, SIGNAL(clicked()), this, SLOT(testLogin()) );
    ui.serverLineEdit->setText( Settings::serverUrl() );
    ui.usernameLineEdit->setText( Settings::username() );
    connect( this, SIGNAL(okClicked()), SLOT(save()) );
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::setPassword( const QString& password )
{
    ui.passwordLineEdit->setText( password );
}

QString ConfigDialog::password() const
{
    return ui.passwordLineEdit->text();
}

void ConfigDialog::save()
{
    Settings::setServerUrl( ui.serverLineEdit->text() );
    Settings::setUsername( ui.usernameLineEdit->text() );
}

void ConfigDialog::testLogin()
{
    QPointer<TestLoginDialog> testLoginDialog( new TestLoginDialog( this ) );
    testLoginDialog->setAttribute( Qt::WA_DeleteOnClose );
    testLoginDialog->startTest( ui.serverLineEdit->text(), ui.usernameLineEdit->text(), ui.passwordLineEdit->text()  );
    testLoginDialog->exec();
}
