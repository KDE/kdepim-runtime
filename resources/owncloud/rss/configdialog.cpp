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
    ListNodeJob* job = new ListNodeJob( ListNodeJob::Folders, this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(jobCompleted(KJob*)) );
    job->setUrl( url );
    job->setUsername( username );
    job->setPassword( password );
    job->start();
}

void TestLoginDialog::jobCompleted( KJob* j )
{
    ListNodeJob* job = qobject_cast<ListNodeJob*>( j );
    Q_ASSERT( job );

    if ( job->error() == KJob::NoError ) {
        m_textBrowser->append( i18n("Test successful") );
    } else {
        m_textBrowser->append( i18n("Error: %1", job->errorString() ) );
    }
}

ConfigDialog::ConfigDialog( const QString& resourceId, QWidget *parent )
    : KDialog( parent )
    , m_resourceId( resourceId )
{
    ui.setupUi( mainWidget() );
    connect( ui.testButton, SIGNAL(clicked()), this, SLOT(testLogin()) );
    ui.owncloudServerLineEdit->setText( Settings::owncloudServerUrl() );
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
    Settings::setOwncloudServerUrl( ui.owncloudServerLineEdit->text() );
    Settings::setUsername( ui.usernameLineEdit->text() );
}

void ConfigDialog::testLogin()
{
    QPointer<TestLoginDialog> testLoginDialog( new TestLoginDialog( this ) );
    testLoginDialog->setAttribute( Qt::WA_DeleteOnClose );
    testLoginDialog->startTest( ui.owncloudServerLineEdit->text(), ui.usernameLineEdit->text(), ui.passwordLineEdit->text()  );
    testLoginDialog->exec();
}

#include "configdialog.moc"
