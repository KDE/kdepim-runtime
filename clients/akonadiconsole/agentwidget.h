/*
    This file is part of Akonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef AGENTWIDGET_H
#define AGENTWIDGET_H

#include <QtGui/QWidget>

#include "agentmanagerinterface.h"

namespace PIM {
class AgentInstanceView;
}

class AgentWidget : public QWidget
{
  Q_OBJECT

  public:
    AgentWidget( QWidget *parent = 0 );

  private Q_SLOTS:
    void addAgent();
    void removeAgent();

  private:
    PIM::AgentInstanceView *mView;
    org::kde::Akonadi::AgentManager *mManager;
};

#endif
