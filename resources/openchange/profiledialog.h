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

#ifndef PROFILEDIALOG_H
#define PROFIDEDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>

class OCResource;

class ProfileDialog : public QDialog
{
  Q_OBJECT

  public:
    explicit ProfileDialog( OCResource *resource, QWidget *parent = 0 );

  private Q_SLOTS:
    void addNewProfile();
    void deleteSelectedProfile();
    void editExistingProfile();
    void setAsDefaultProfile();

  private:
    void fillProfileList();
    QString selectedProfileName();

    OCResource *m_resource;
    QListWidget *m_listOfProfiles;

    QPushButton *m_setAsDefaultButton;
    QPushButton *m_editButton;
    QPushButton *m_removeButton;
};

#endif
