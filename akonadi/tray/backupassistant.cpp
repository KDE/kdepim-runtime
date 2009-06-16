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

#include "backupassistant.h"
#include "backup.h"

#include <QLabel>
#include <QDateTime>
#include <QPushButton>

#include <KAssistantDialog>
#include <KDebug>
#include <KFileDialog>
#include <KLocale>
#include <KVBox>

BackupAssistant::BackupAssistant( QWidget *parent ) : KAssistantDialog( parent )
{
    m_backup = new Backup( this );
    connect( m_backup, SIGNAL( completed( bool ) ), SLOT( slotBackupComplete( bool ) ) );
    bool possible = m_backup->possible();

    KVBox *box1 = new KVBox( this );
    QLabel *label1 = new QLabel( box1 );
    label1->setWordWrap( true );
    if ( !possible ) {
        label1->setText( '\n' + i18n( "The backup can not be made. Either the mysqldump application "
                                      "is not installed, or the bzip2 application is not found. "
                                      "Please install those and make sure they can be found in "
                                      "the current path. Restart this Assistant when this is fixed." ) );
    } else {
        label1->setText( '\n' + i18n( "Please select the file where to store "
                                      "the backup, give it the extension .tar.bz2" ) + "\n\n" );

        m_selectFileButton = new QPushButton( i18n( "&Click Here to Select the Backup Location..." ), box1 );
        connect( m_selectFileButton, SIGNAL( clicked( bool ) ), SLOT( slotSelectFile() ) );

        QLabel *label2 = new QLabel( "\n\n" + i18n( "Press 'Next' to start the Backup" ), box1 );
        label2->setWordWrap( true );
        label2->setAlignment( Qt::AlignRight );
    }
    m_page1 = new KPageWidgetItem( box1, i18n( "Welcome to the Backup Assistant" ) );
    setValid( m_page1, false );

    m_backupProgressLabel = new QLabel( this );
    m_backupProgressLabel->setWordWrap( true );
    m_page2 = new KPageWidgetItem( m_backupProgressLabel, i18n( "Making the backup" ) );
    setValid( m_page2, false );

    addPage( m_page1 );
    addPage( m_page2 );
    showButton( KDialog::Help, false );

    connect( this, SIGNAL( currentPageChanged( KPageWidgetItem*, KPageWidgetItem* ) ),
             SLOT( slotPageChanged( KPageWidgetItem*, KPageWidgetItem* ) ) );
}

void BackupAssistant::slotSelectFile()
{
    QString file = QString( "akonadibackup-" +
                            QDateTime::currentDateTime().toString( "yyyyMMdd" )  + ".tar.bz2" );

    // Build one special, as we want the keyword /and/ a proposed filename
    KFileDialog dlg( KUrl( "kfiledialog://BackupDir" ), QString(), this );
    dlg.setSelection( file );
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setMode( KFile::File );
    dlg.setWindowTitle( i18n( "Save As" ) );
    dlg.exec();

    m_filename = dlg.selectedFile();
    if ( !m_filename.isEmpty() ) {
        m_selectFileButton->setText( m_filename );
        setValid( m_page1, true );
    }
}

void BackupAssistant::slotPageChanged( KPageWidgetItem *current, KPageWidgetItem* )
{
    if ( current != m_page2 )
        return;

    setValid( m_page2, false );
    m_backupProgressLabel->setText( i18n( "Please be patient, the backup is being created..." ) );
    m_backup->create( m_filename );
}

void BackupAssistant::slotBackupComplete( bool ok )
{
    if ( ok ) {
        m_backupProgressLabel->setText( i18n( "The backup has been made. Please verify manually "
                                              "if the backup is complete. Also note that KWallet "
                                              "stored passwords are not in the backup, you might "
                                              "want to verify you have a backup of those elsewhere." ) );
        setValid( m_page2, true );
    } else
        m_backupProgressLabel->setText( i18n( "The backup process ended unexpectedly. Please "
                                              "report a bug, so we can find out what the cause is." ) );
}

#include "backupassistant.moc"
