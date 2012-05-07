/*
  Copyright (c) 2010 Laurent Montel <montel@kde.org>
  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef KOLABPROXY_SETUPKOLAB_H
#define KOLABPROXY_SETUPKOLAB_H

#include "ui_changeformat.h"
#include "ui_kolabsettings.h"

#include <Akonadi/AgentInstance>

#include <KDialog>

class KolabProxyResource;

class KJob;

class SetupKolab : public KDialog
{
  Q_OBJECT

  public:
    SetupKolab( KolabProxyResource *parentResource, WId parent );
    ~SetupKolab();

  protected:
    void initConnection();
    void updateCombobox();

  protected slots:
    void slotLaunchWizard();
    void slotInstanceAddedRemoved();
    void slotCreateDefaultKolabCollections();
    void slotShowUpgradeDialog();
    void slotDoUpgrade();
    void slotSelectedAccountChanged();
    void slotUpgradeProgress( KJob *, ulong );
    void slotUpgradeDone( KJob * );

  private:
    QMap<QString, Akonadi::AgentInstance> m_agentList;
    Ui::SetupKolabView *m_ui;
    Ui::ChangeFormatView *m_versionUi;
    KolabProxyResource *m_parentResource;
};

#endif /* SETUPKOLAB_H */

