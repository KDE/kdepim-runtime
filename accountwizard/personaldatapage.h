/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Tom Albers <toma@kde.org>
    Copyright (c) 2012 Laurent Montel <montel@kde.org>

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

#ifndef PERSONALDATA_H
#define PERSONALDATA_H

#include "page.h"
#include "setupmanager.h"
#include "dialog.h"

#include "ui_personaldatapage.h"

class Ispdb;

class PersonalDataPage : public Page
{
    Q_OBJECT
public:
    explicit PersonalDataPage(Dialog *parent = 0);
    void setHideOptionInternetSearch(bool);

    virtual void leavePageNext();
    virtual void leavePageNextRequested();

private slots:
    void ispdbSearchFinished(bool ok);
    void slotTextChanged();
    void slotCreateAccountClicked();
    void slotRadioButtonClicked(QAbstractButton *button);
    void slotSearchType(const QString &);

signals:
    void manualWanted(bool);

private:
    void automaticConfigureAccount();
    void configureSmtpAccount();
    void configureImapAccount();
    void configurePop3Account();

    Ui::PersonalDataPage ui;
    Ispdb *mIspdb;
    SetupManager *mSetupManager;
};

#endif
