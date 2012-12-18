/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef LOADPAGE_H
#define LOADPAGE_H

#include "page.h"

#include "ui_loadpage.h"

namespace Kross {
class Action;
}

class LoadPage : public Page
{
  Q_OBJECT
  public:
    explicit LoadPage( KAssistantDialog *parent );

    virtual void enterPageNext();
    virtual void enterPageBack();

    void exportObject( QObject *object, const QString &name );

  Q_SIGNALS:
    void aboutToStart();

  private:
    Ui::LoadPage ui;
    QVector< QPair< QObject*, QString > > m_exportedObjects;
    Kross::Action* m_action;
};

#endif
