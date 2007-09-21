/*
    Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

    Significant amounts of this code adapted from the openchange client utility,
    which is Copyright (C) Julien Kerihuel 2007 <j.kerihuel@openchange.org>.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef PROFILEEDITDIALOG_H
#define PROFILEEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class ProfileEditDialog : public QDialog
{
  Q_OBJECT

  public:
    ProfileEditDialog( QWidget *parent = 0,
                       const QString &profileName = QString(),
                       const QString &userName = QString(),
                       const QString &password = QString(),
                       const QString &serverAddress = QString(),
                       const QString &workstation = QString(),
                       const QString &domain = QString() );


  private Q_SLOTS:
    void commitProfile();
    void checkIfComplete();

  private:
    QLineEdit *m_profileNameEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_addressEdit;
    QLineEdit *m_workstationEdit;
    QLineEdit *m_domainEdit;

    QPushButton *m_okButton;
};

#endif
