/*4
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright 2009 Thomas McGuire <mcguire@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _ACCOUNT_DIALOG_H_
#define _ACCOUNT_DIALOG_H_

#include "ui_popsettings.h"

#include <Akonadi/Collection>

#include <KDialog>
#include <KLineEdit>

#include <QRegExpValidator>

namespace MailTransport {
class ServerTest;
}

class POP3Resource;
class KJob;

class AccountDialog : public KDialog, private Ui::PopPage
{
  Q_OBJECT

  public:
    AccountDialog( POP3Resource *parentResource, WId parentWindow );
    virtual ~AccountDialog();

  private slots:
    virtual void slotOk();
    void slotEnablePopInterval( bool state );
    void slotFontChanged();
    void slotLeaveOnServerClicked();
    void slotEnableLeaveOnServerDays( bool state );
    void slotEnableLeaveOnServerCount( bool state );
    void slotEnableLeaveOnServerSize( bool state );
    void slotFilterOnServerClicked();
    void slotPipeliningClicked();
    void slotPopEncryptionChanged(int);
    void slotPopPasswordChanged( const QString& text );
    void slotCheckPopCapabilities();
    void slotPopCapabilities( QList<int> );
    void slotLeaveOnServerDaysChanged( int value );
    void slotLeaveOnServerCountChanged( int value );
    void slotFilterOnServerSizeChanged( int value );

    void targetCollectionReceived( Akonadi::Collection::List collections );
    void localFolderRequestJobFinished( KJob *job );

  private:
    void setupWidgets();
    void loadSettings();
    void saveSettings();
    void checkHighest( QButtonGroup * );
    void enablePopFeatures();

  private:
    POP3Resource *mParentResource;
    QButtonGroup *encryptionButtonGroup;
    QButtonGroup *authButtonGroup;
    MailTransport::ServerTest *mServerTest;
    QRegExpValidator mValidator;
    bool mServerTestFailed;
};

#endif
