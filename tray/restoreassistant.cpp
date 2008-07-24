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

#include "restoreassistant.h"
#include "restore.h"

#include <QLabel>
#include <QPushButton>

#include <KAssistantDialog>
#include <KDebug>
#include <KFileDialog>
#include <KLocale>
#include <KVBox>

RestoreAssistant::RestoreAssistant( QWidget *parent ) : KAssistantDialog( parent )
{
    m_restore = new Restore( this );
    connect( m_restore, SIGNAL( completed( bool ) ), SLOT( slotRestoreComplete( bool ) ) );
    bool possible = m_restore->possible();

    KVBox *box1 = new KVBox( this );
    QLabel *label1 = new QLabel( box1 );
    label1->setWordWrap( true );
    if ( !possible ) {
        label1->setText( '\n' + i18n( "The backup can not be restored. Either the mysql application "
                                      "is not installed, or the bzip2 application is not found. "
                                      "Please install those and make sure they can be found in "
                                      "the current path. Restart this Assistant when this is fixed." ) );
    } else {
        label1->setText( '\n' + i18n( "Please select the file to restore. Note that restoring a "
                                      "backup will overwrite all existing data. You might want to "
                                      "make a backup first and please consider closing all Akonadi "
                                      "applications (but do not stop the akonadi server)." ) + "\n\n" );

        m_selectFileButton = new QPushButton( i18n( "&Click here to select the file to restore..." ), box1 );
        connect( m_selectFileButton, SIGNAL( clicked( bool ) ), SLOT( slotSelectFile() ) );

        QLabel *label2 = new QLabel( "\n\n" + i18n( "Press 'Next' to start the Restore" ), box1 );
        label2->setWordWrap( true );
        label2->setAlignment( Qt::AlignRight );
    }
    m_page1 = new KPageWidgetItem( box1, i18n( "Welcome to the Restore Assistant" ) );
    setValid( m_page1, false );

    m_restoreProgressLabel = new QLabel( this );
    m_restoreProgressLabel->setWordWrap( true );
    m_page2 = new KPageWidgetItem( m_restoreProgressLabel, i18n( "Restoring" ) );
    setValid( m_page2, false );

    addPage( m_page1 );
    addPage( m_page2 );
    showButton( KDialog::Help, false );

    connect( this, SIGNAL( currentPageChanged( KPageWidgetItem*, KPageWidgetItem* ) ),
             SLOT( slotPageChanged( KPageWidgetItem*, KPageWidgetItem* ) ) );
}

void RestoreAssistant::slotSelectFile()
{
    m_filename = KFileDialog::getOpenFileName( KUrl( "kfiledialog://BackupDir" ) );
    if ( !m_filename.isEmpty() ) {
        m_selectFileButton->setText( m_filename );
        setValid( m_page1, true );
    }
}

void RestoreAssistant::slotPageChanged( KPageWidgetItem *current, KPageWidgetItem* )
{
    if ( current != m_page2 )
        return;

    setValid( m_page2, false );
    m_restoreProgressLabel->setText( i18n( "Please be patient, the backup is being restored..." ) );
    m_restore->restore( m_filename );
}

void RestoreAssistant::slotRestoreComplete( bool ok )
{
    if ( ok ) {
        m_restoreProgressLabel->setText( i18n( "The backup has been restored. Note that KWallet "
                                               "stored passwords are not restored, so applications might "
                                               "ask for those." ) );
        setValid( m_page2, true );
    } else
        m_restoreProgressLabel->setText( i18n( "The restore process ended unexpectedly. Please "
                                               "report a bug, so we can find out what the cause is." ) );
}

#include "restoreassistant.moc"
