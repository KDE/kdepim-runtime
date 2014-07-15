/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "settingsdialog.h"
#include "settings.h"

#include <KDE/KLocalizedString>
#include <KDE/KListWidget>
#include <KDE/KPushButton>
#include <KDE/KDateComboBox>

#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtGui/QListWidget>
#include <QtGui/QGroupBox>
#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtCore/QPointer>

#include <LibKGAPI2/Account>
#include <LibKGAPI2/AuthJob>
#include <LibKGAPI2/Calendar/Calendar>
#include <LibKGAPI2/Calendar/CalendarFetchJob>
#include <LibKGAPI2/Tasks/TaskList>
#include <LibKGAPI2/Tasks/TaskListFetchJob>

using namespace KGAPI2;


SettingsDialog::SettingsDialog( GoogleAccountManager *accountManager, WId windowId, GoogleResource *parent ):
    GoogleSettingsDialog( accountManager, windowId, parent )
{
    connect( this, SIGNAL(accepted()),
             this, SLOT(saveSettings()) );
    connect( this, SIGNAL(currentAccountChanged(QString)),
             this, SLOT(slotCurrentAccountChanged(QString)) );


    m_calendarsBox = new QGroupBox( i18n( "Calendars" ), this );
    mainWidget()->layout()->addWidget( m_calendarsBox );

    QVBoxLayout *vbox = new QVBoxLayout( m_calendarsBox );

    m_calendarsList = new KListWidget( m_calendarsBox );
    vbox->addWidget( m_calendarsList, 1 );

    m_reloadCalendarsBtn = new KPushButton( KIcon( QLatin1String("view-refresh") ), i18n( "Reload" ), m_calendarsBox );
    vbox->addWidget( m_reloadCalendarsBtn );
    connect( m_reloadCalendarsBtn, SIGNAL(clicked(bool)),
             this, SLOT(slotReloadCalendars()) );

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout( hbox );

    m_eventsLimitLabel = new QLabel( i18nc( "Followed by a date picker widget", "&Fetch only new events since" ), this );
    hbox->addWidget( m_eventsLimitLabel );

    m_eventsLimitCombo = new KDateComboBox( this );
    m_eventsLimitLabel->setBuddy( m_eventsLimitCombo );
    m_eventsLimitCombo->setMaximumDate( QDate::currentDate() );
    m_eventsLimitCombo->setMinimumDate( QDate::fromString( QLatin1String( "2000-01-01" ), Qt::ISODate ) );
    m_eventsLimitCombo->setOptions( KDateComboBox::EditDate | KDateComboBox::SelectDate |
                                    KDateComboBox::DatePicker | KDateComboBox::WarnOnInvalid );
    if( Settings::self()->eventsSince().isEmpty() ) {
        const QString ds = QString::fromLatin1( "%1-01-01" ).arg( QString::number( QDate::currentDate().year() - 3 ) );
        m_eventsLimitCombo->setDate( QDate::fromString( ds, Qt::ISODate ) );
    } else {
        m_eventsLimitCombo->setDate( QDate::fromString( Settings::self()->eventsSince(), Qt::ISODate ) );
    }
    hbox->addWidget( m_eventsLimitCombo );

    m_taskListsBox = new QGroupBox( i18n( "Tasklists" ), this );
    mainWidget()->layout()->addWidget( m_taskListsBox );

    vbox = new QVBoxLayout( m_taskListsBox );

    m_taskListsList = new KListWidget( m_taskListsBox );
    vbox->addWidget( m_taskListsList, 1 );

    m_reloadTaskListsBtn = new KPushButton( KIcon( QLatin1String("view-refresh") ), i18n( "Reload" ), m_taskListsBox );
    vbox->addWidget( m_reloadTaskListsBtn );
    connect( m_reloadTaskListsBtn, SIGNAL(clicked(bool)),
             this, SLOT(slotReloadTaskLists()) );
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::saveSettings()
{
    const AccountPtr account = currentAccount();
    if ( !currentAccount() ) {
        Settings::self()->setAccount( QString() );
        Settings::self()->setCalendars( QStringList() );
        Settings::self()->setTaskLists( QStringList() );
        Settings::self()->setEventsSince( QString() );
        Settings::self()->writeConfig();
        return;
    }

    Settings::self()->setAccount( account->accountName() );

    QStringList calendars;
    for ( int i = 0; i < m_calendarsList->count(); i++ ) {
        QListWidgetItem *item = m_calendarsList->item( i );

        if ( item->checkState() == Qt::Checked ) {
            calendars.append( item->data( Qt::UserRole ).toString() );
        }
    }
    Settings::self()->setCalendars( calendars );
    if ( m_eventsLimitCombo->isValid() ) {
        Settings::self()->setEventsSince( m_eventsLimitCombo->date().toString( Qt::ISODate ) );
    }

    QStringList taskLists;
    for ( int i = 0; i < m_taskListsList->count(); i++ ) {
        QListWidgetItem *item = m_taskListsList->item( i );

        if ( item->checkState() == Qt::Checked ) {
            taskLists.append( item->data( Qt::UserRole ).toString() );
        }
    }
    Settings::self()->setTaskLists( taskLists );


    Settings::self()->writeConfig();
}

void SettingsDialog::slotCurrentAccountChanged( const QString &accountName )
{
    if ( accountName.isEmpty() ) {
        return;
    }

    slotReloadCalendars();
    slotReloadTaskLists();
}

void SettingsDialog::slotReloadCalendars()
{
    const AccountPtr account = currentAccount();
    if ( !account ) {
        return;
    }

    CalendarFetchJob *fetchJob = new CalendarFetchJob( account, this );
    connect( fetchJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotCalendarsRetrieved(KGAPI2::Job*)) );

    m_calendarsBox->setDisabled( true );
    m_calendarsList->clear();
}

void SettingsDialog::slotReloadTaskLists()
{
    const AccountPtr account = currentAccount();
    if ( !account ) {
        return;
    }

    TaskListFetchJob *fetchJob = new TaskListFetchJob( account, this );
    connect( fetchJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotTaskListsRetrieved(KGAPI2::Job*)) );

    m_taskListsBox->setDisabled( true );
    m_taskListsList->clear();
}

void SettingsDialog::slotCalendarsRetrieved( Job *job )
{
    if ( !handleError( job ) || !currentAccount() ) {
        m_calendarsBox->setEnabled( true );
        return;
    }

    FetchJob *fetchJob = qobject_cast<FetchJob*>(job);
    ObjectsList objects = fetchJob->items();

    QStringList activeCalendars;
    if ( currentAccount()->accountName() == Settings::self()->account() ) {
        activeCalendars = Settings::self()->calendars();
    }
    Q_FOREACH( const ObjectPtr &object, objects ) {
        CalendarPtr calendar = object.dynamicCast<Calendar>();

        QListWidgetItem *item = new QListWidgetItem( calendar->title() );
        item->setData( Qt::UserRole, calendar->uid() );
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable );
        item->setCheckState( ( activeCalendars.isEmpty() || activeCalendars.contains( calendar->uid() ) ) ? Qt::Checked : Qt::Unchecked );
        m_calendarsList->addItem( item );

    }

    m_calendarsBox->setEnabled( true );
}

void SettingsDialog::slotTaskListsRetrieved( Job *job )
{
    if ( !handleError( job ) || !currentAccount() ) {
        m_taskListsBox->setEnabled( true );
        return;
    }

    FetchJob *fetchJob = qobject_cast<FetchJob*>(job);
    ObjectsList objects = fetchJob->items();

    QStringList activeTaskLists;
    if ( currentAccount()->accountName() == Settings::self()->account()) {
        activeTaskLists = Settings::self()->taskLists();
    }
    Q_FOREACH( const ObjectPtr &object, objects ) {
        TaskListPtr taskList = object.dynamicCast<TaskList>();

        QListWidgetItem *item = new QListWidgetItem( taskList->title() );
        item->setData( Qt::UserRole, taskList->uid() );
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable );
        item->setCheckState( ( activeTaskLists.isEmpty() || activeTaskLists.contains( taskList->uid() ) ) ? Qt::Checked : Qt::Unchecked );
        m_taskListsList->addItem( item );
    }

    m_taskListsBox->setEnabled( true );
}
