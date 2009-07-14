/*   -*- c++ -*-
 *   accountdialog.h
 *
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
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

#include <kdialog.h>
#include <klineedit.h>

#include <QPointer>
#include <QList>

class QRegExpValidator;
class QCheckBox;
class QPushButton;
class QLabel;
class QRadioButton;
class QToolButton;
class KIntNumInput;
class KMAccount;
class KMFolder;
class QButtonGroup;
class QGroupBox;

namespace MailTransport {
class ServerTest;
}

namespace KPIMIdentities {
class IdentityCombo;
}

class SieveConfigEditor;
class FolderRequester;

class AccountDialog : public KDialog
{
  Q_OBJECT

  public:
    AccountDialog( QWidget *parent=0 );
    virtual ~AccountDialog();
  private:

    struct PopWidgets
    {
      Ui::PopPage ui;
      QButtonGroup *encryptionButtonGroup;
      QButtonGroup *authButtonGroup;
    };

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
    void slotIdentityCheckboxChanged();

  private:
    void makePopAccountPage();
    void setupSettings();
    void saveSettings();
    void checkHighest( QButtonGroup * );
    void enablePopFeatures();
    void initAccountForConnect();
    const QString namespaceListToString( const QStringList& list );

  private:
    PopWidgets   mPop;
    KMAccount    *mAccount;
    QList<QPointer<KMFolder> > mFolderList;
    QStringList  mFolderNames;
    MailTransport::ServerTest *mServerTest;
    QRegExpValidator *mValidator;
    bool mServerTestFailed;
};

#endif
