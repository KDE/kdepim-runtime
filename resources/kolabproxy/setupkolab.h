/*
    Copyright (c) 2010 Laurent Montel <montel@kde.org>

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

#ifndef SETUPKOLAB_H
#define SETUPKOLAB_H

#include <KDialog>
#include "ui_kolabsettings.h"

class SetupKolab : public KDialog
{
  Q_OBJECT

public:
  SetupKolab( WId parent );
  ~SetupKolab();

protected:
  void initConnection();
  void updateCombobox();

protected slots:
  void slotLaunchWizard();
  void slotInstanceAddedRemoved();

private:
  Ui::SetupKolabView *m_ui;
};


#endif /* SETUPKOLAB_H */

