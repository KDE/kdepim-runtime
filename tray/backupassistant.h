/* This file is part of the KDE project

   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#ifndef BACKUPASSISTANT_H
#define BACKUPASSISTANT_H

#include <KAssistantDialog>
class Backup;
class QLabel;
class QPushButton;

/**
 * Use this class to create a backup assistant.
 */
class BackupAssistant : public KAssistantDialog
{
    Q_OBJECT
public:
    /**
     * Constructor
     */
    BackupAssistant( QWidget* );

private slots:
    void slotSelectFile( );
    void slotPageChanged( KPageWidgetItem*, KPageWidgetItem* );
    void slotBackupComplete( bool ok );

private:
    Backup *m_backup;
    KPageWidgetItem *m_page1;
    KPageWidgetItem *m_page2;
    QLabel *m_backupProgressLabel;
    QPushButton *m_selectFileButton;
    QString m_filename;
};

#endif

