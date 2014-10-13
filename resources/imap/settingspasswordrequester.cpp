/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                         a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#include "settingspasswordrequester.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <QDialog>

#include <mailtransport/transportbase.h>
#include <kwindowsystem.h>
#include <QDebug>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "imapresourcebase.h"
#include "settings.h"

SettingsPasswordRequester::SettingsPasswordRequester( ImapResourceBase *resource, QObject *parent )
  : PasswordRequesterInterface( parent ), m_resource( resource ), m_requestDialog( 0 ), m_settingsDialog( 0 )
{

}

SettingsPasswordRequester::~SettingsPasswordRequester()
{
  cancelPasswordRequests();
}

void SettingsPasswordRequester::requestPassword( RequestType request, const QString &serverError )
{
  if ( request == WrongPasswordRequest ) {
    QMetaObject::invokeMethod( this, "askUserInput", Qt::QueuedConnection, Q_ARG(QString, serverError) );
  } else {
    connect( m_resource->settings(), SIGNAL(passwordRequestCompleted(QString,bool)),
             this, SLOT(onPasswordRequestCompleted(QString,bool)) );
    m_resource->settings()->requestPassword();
  }
}

void SettingsPasswordRequester::askUserInput( const QString &serverError )
{
  // the credentials were not ok, allow to retry or change password
  if ( m_requestDialog ) {
    qDebug() << "Password request dialog is already open";
    return;
  }
  QWidget *parent = QWidget::find(m_resource->winIdForDialogs());
  QString text = i18n( "The server for account \"%2\" refused the supplied username and password. "
                                                     "Do you want to go to the settings, have another attempt "
                                                     "at logging in, or do nothing?\n\n"
                                                     "%1", serverError, m_resource->name() );
  QDialog *dialog = new QDialog(parent, Qt::Dialog);
  dialog->setWindowTitle(i18n( "Could Not Authenticate" ));
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::No|QDialogButtonBox::Yes);
  buttonBox->button(QDialogButtonBox::Yes)->setDefault(true);

  buttonBox->button(QDialogButtonBox::Yes)->setText(i18n( "Account Settings" ));
  buttonBox->button(QDialogButtonBox::No)->setText(i18nc( "Input username/password manually and not store them", "Try Again" ));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(buttonBox->button(QDialogButtonBox::Yes), &QPushButton::clicked, this, &SettingsPasswordRequester::slotYesClicked);
  connect(buttonBox->button(QDialogButtonBox::No), &QPushButton::clicked, this, &SettingsPasswordRequester::slotNoClicked);
  connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &SettingsPasswordRequester::slotCancelClicked);

  connect(dialog, &QDialog::destroyed, this, &SettingsPasswordRequester::onDialogDestroyed);
  m_requestDialog = dialog;
  KWindowSystem::setMainWindow(dialog, m_resource->winIdForDialogs());
  bool checkboxResult = false;
  KMessageBox::createKMessageBox(dialog, buttonBox, QMessageBox::Information,
                                       text, QStringList(),
                                       QString(),
                                       &checkboxResult, KMessageBox::NoExec);
  dialog->show();
}

void SettingsPasswordRequester::onDialogDestroyed()
{
  m_requestDialog = 0;
}

void SettingsPasswordRequester::slotNoClicked()
{
    connect( m_resource->settings(), SIGNAL(passwordRequestCompleted(QString,bool)),
             this, SLOT(onPasswordRequestCompleted(QString,bool)) );
    m_resource->settings()->requestManualAuth();
    m_requestDialog = 0;
}

void SettingsPasswordRequester::slotYesClicked()
{
    if (!m_settingsDialog) {
      QDialog *dialog = m_resource->createConfigureDialog(m_resource->winIdForDialogs());
      connect(dialog, &QDialog::finished, this, &SettingsPasswordRequester::onSettingsDialogFinished);
      m_settingsDialog = dialog;
      dialog->show();
    }
    m_requestDialog = 0;
}

void SettingsPasswordRequester::slotCancelClicked()
{
    emit done( UserRejected );
    m_requestDialog = 0;
}

void SettingsPasswordRequester::onSettingsDialogFinished(int result)
{
  m_settingsDialog = 0;
  if (result == QDialog::Accepted) {
    emit done( ReconnectNeeded );
  } else {
    emit done( UserRejected );
  }
}

void SettingsPasswordRequester::cancelPasswordRequests()
{
  if (m_requestDialog) {
    if (m_requestDialog->close()) {
        m_requestDialog = 0;
    }
  }
}

void SettingsPasswordRequester::onPasswordRequestCompleted( const QString &password, bool userRejected )
{
  disconnect( m_resource->settings(), SIGNAL(passwordRequestCompleted(QString,bool)),
              this, SLOT(onPasswordRequestCompleted(QString,bool)) );

  if ( userRejected ) {
    emit done( UserRejected );
  } else if ( password.isEmpty() && (m_resource->settings()->authentication() != MailTransport::Transport::EnumAuthenticationType::GSSAPI) ) {
    emit done( EmptyPasswordEntered );
  } else {
    emit done( PasswordRetrieved, password );
  }
}
