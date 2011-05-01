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

#include "davutils.h"

#include <QtGui/QWizard>
#include <QtGui/QWizardPage>

class KJob;
class KLineEdit;
class KTextBrowser;

class QButtonGroup;
class QCheckBox;
class QLabel;
class QRadioButton;

class SetupWizard : public QWizard
{
  Q_OBJECT

  public:
    SetupWizard( QWidget *parent = 0 );

    class Url
    {
      public:
        typedef QList<Url> List;

        DavUtils::Protocol protocol;
        QString url;
        QString userName;
    };

    Url::List urls() const;
    QString displayName() const;
};

class CredentialsPage : public QWizardPage
{
  public:
    CredentialsPage( QWidget *parent = 0 );

  private:
    KLineEdit *mUserName;
};

class ServerTypePage : public QWizardPage
{
  public:
    ServerTypePage( QWidget *parent = 0 );

    virtual bool isComplete() const;

  private:
    QButtonGroup *mServerGroup;
    QRadioButton *mEGroupwareServer;
    QRadioButton *mScalixServer;
};

class ConnectionPage : public QWizardPage
{
  Q_OBJECT

  public:
    ConnectionPage( QWidget *parent = 0 );

  private slots:
    void urlElementChanged();

  private:
    KLineEdit *mHost;
    KLineEdit *mPath;
    QLabel *mFullUrlPreview;
    QCheckBox *mUseSecureConnection;
};

class CheckPage : public QWizardPage
{
  Q_OBJECT

  public:
    CheckPage( QWidget *parent = 0 );

  private Q_SLOTS:
    void checkConnection();
    void onFetchDone( KJob* );

  private:
    KTextBrowser *mStatusLabel;
};

#endif
