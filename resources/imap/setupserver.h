/* This file is part of the KDE project
   Copyright (C) 2009 Kevin Ottens <ervin@kde.org>
   Copyright (C) 2006-2008 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef SETUPSERVER_H
#define SETUPSERVER_H

#include <klineedit.h>
#include <kdialog.h>

class QProgressBar;
class QButtonGroup;
class QGroupBox;
class QLabel;
class QRadioButton;
class QPushButton;

namespace Ui
{
class SetupServerView;
}

namespace MailTransport
{
class ServerTest;
}

/**
 * @class SetupServer
 * These contain the account settings
 * @author Tom Albers <tomalbers@kde.nl>
 */
class SetupServer : public KDialog
{
  Q_OBJECT

public:
  /**
   * Constructor
   * @param parent Parent WId
   */
  SetupServer( WId parent );

  /**
   * Destructor
   */
  ~SetupServer();

private slots:
  /**
   * Call this if you want the settings saved from this page.
   */
  void applySettings();

private:
  void readSettings();

  Ui::SetupServerView *m_ui;
  MailTransport::ServerTest *m_serverTest;

private slots:
  void slotTest();
  void slotFinished( QList<int> testResult );
  void slotTestChanged();
  void slotComplete();
  void slotSafetyChanged();
};

#endif
