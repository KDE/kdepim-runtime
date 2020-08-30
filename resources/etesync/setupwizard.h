/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QLabel>
#include <QProgressBar>
#include <QWizard>
#include <QWizardPage>

#include "etesyncclientstate.h"

class KJob;
class QLineEdit;
class QTextBrowser;

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QRadioButton;
class KPasswordLineEdit;

class SetupWizard : public QWizard
{
    Q_OBJECT

public:
    explicit SetupWizard(EteSyncClientState *clientState, QWidget *parent = nullptr);

    enum
    {
        W_LoginPage,
        W_EncryptionPasswordPage
    };

    EteSyncClientState *mClientState = nullptr;

public Q_SLOTS:
    void manualNext();
};

class LoginPage : public QWizardPage
{
public:
    explicit LoginPage(QWidget *parent = nullptr);
    int nextId() const override;
    void initializePage() override;
    bool validatePage() override;

    void showProgressBar()
    {
        mProgressBar->setVisible(true);
    }

    void hideProgressBar()
    {
        mProgressBar->setVisible(false);
    }

    void setLoginResult(bool loginResult)
    {
        mLoginResult = loginResult;
    }

    void setUserInfoResult(bool userInfoResult)
    {
        mUserInfoResult = userInfoResult;
    }

    void setErrorCode(int errorCode)
    {
        mErrorCode = errorCode;
    }

    void setErrorMessage(QString errorMessage)
    {
        mErrorMessage = errorMessage;
    }

private:
    QLineEdit *mUserName = nullptr;
    KPasswordLineEdit *mPassword = nullptr;
    QCheckBox *mAdvancedSettings = nullptr;
    QLineEdit *mServerUrl = nullptr;
    QLabel *mLoginLabel = nullptr;
    QProgressBar *mProgressBar;
    bool mIsInitialized = false;
    bool mLoginResult = false;
    bool mUserInfoResult = false;
    int mErrorCode;
    QString mErrorMessage;
};

class EncryptionPasswordPage : public QWizardPage
{
public:
    explicit EncryptionPasswordPage(QWidget *parent = nullptr);
    int nextId() const override;
    void initializePage() override;
    bool validatePage() override;

private:
    KPasswordLineEdit *mEncryptionPassword = nullptr;
    QLabel *mEncryptionPasswordLabel = nullptr;
};

#endif
