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

#include <KDAV/Enums>

#include <QWizard>
#include <QWizardPage>
#include <QLabel>

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
    explicit SetupWizard(QWidget *parent = nullptr);

    enum {
        W_CredentialsPage,
        W_PredefinedProviderPage,
        W_ServerTypePage,
        W_ConnectionPage,
        W_CheckPage
    };

    class Url
    {
    public:
        typedef QVector<Url> List;

        KDAV::Protocol protocol;
        QString url;
        QString userName;
        QString password;
    };

    Url::List urls() const;
    QString displayName() const;
};

class PredefinedProviderPage : public QWizardPage
{
public:
    explicit PredefinedProviderPage(QWidget *parent = nullptr);

    void initializePage() override;
    int nextId() const override;

private:
    QLabel *mLabel = nullptr;
    QButtonGroup *mProviderGroup = nullptr;
    QRadioButton *mUseProvider = nullptr;
    QRadioButton *mDontUseProvider = nullptr;
};

class CredentialsPage : public QWizardPage
{
public:
    explicit CredentialsPage(QWidget *parent = nullptr);
    int nextId() const override;

private:
    QLineEdit *mUserName = nullptr;
    KPasswordLineEdit *mPassword = nullptr;
};

class ServerTypePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ServerTypePage(QWidget *parent = nullptr);

    bool validatePage() override;

private:
    void manualConfigToggled(bool toggled);
    QButtonGroup *mServerGroup = nullptr;
    QComboBox *mProvidersCombo = nullptr;
};

class ConnectionPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ConnectionPage(QWidget *parent = nullptr);

    void initializePage() override;
    void cleanupPage() override;

private:
    void urlElementChanged();
    QFormLayout *mLayout = nullptr;
    QLineEdit *mHost = nullptr;
    QLineEdit *mPath = nullptr;
    QCheckBox *mUseSecureConnection = nullptr;
    QFormLayout *mPreviewLayout = nullptr;
    QLabel *mCalDavUrlLabel = nullptr;
    QLabel *mCalDavUrlPreview = nullptr;
    QLabel *mCardDavUrlLabel = nullptr;
    QLabel *mCardDavUrlPreview = nullptr;
    QLabel *mGroupDavUrlLabel = nullptr;
    QLabel *mGroupDavUrlPreview = nullptr;
};

class CheckPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CheckPage(QWidget *parent = nullptr);

private:
    void checkConnection();
    void onFetchDone(KJob *);
    QTextBrowser *mStatusLabel = nullptr;
};

#endif
