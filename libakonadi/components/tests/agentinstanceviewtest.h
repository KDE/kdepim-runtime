/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#ifndef AGENTINSTANCEVIEWTEST_H
#define AGENTINSTANCEVIEWTEST_H

#include <QtGui/QDialog>

#include "agentinstanceview.h"

class Dialog : public QDialog
{
  Q_OBJECT

  public:
    Dialog( QWidget *parent = 0 );

    virtual void done( int );

  private Q_SLOTS:
    void currentChanged( const QString&, const QString& );

  private:
    Akonadi::AgentInstanceView *mView;
};

#endif
