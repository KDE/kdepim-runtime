/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QLabel>
#include <QProgressBar>
#include <QWizard>
#include <QWizardPage>

#include "etesyncclientstate.h"

class QLineEdit;
class QCheckBox;
class QLabel;
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
