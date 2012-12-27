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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <KPageDialog>

#include "ui_configdialog.h"

namespace KWallet {
    class Wallet;
}

class KConfigDialogManager;

class QTextBrowser;

class TestLoginDialog : public KDialog
{
    Q_OBJECT
public:
    explicit TestLoginDialog( QWidget* parent=0 );
    void startTest( const KUrl& url, const QString& password );

private:
    QTextBrowser* m_textBrowser;
};

class ConfigDialog : public KDialog
{
  Q_OBJECT
public:
    explicit ConfigDialog( const QString& resourceId, QWidget *parent = 0 );
    ~ConfigDialog();

private Q_SLOTS:
    void save();
    void testLogin();
    void walletOpened( bool success );

private:
    Ui::ConfigDialog ui;
    KConfigDialogManager* m_manager;
    KWallet::Wallet* m_wallet;
    QString m_resourceId;
};

#endif
