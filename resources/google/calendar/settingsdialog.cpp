/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

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
#include "calendareditor.h"
#include "settings.h"
#include "tasklisteditor.h"
#include "ui_settingsdialog.h"

#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtGui/QListWidget>

#include <KWindowSystem>
#include <KLocalizedString>
#include <KMessageBox>

#include <libkgoogle/accessmanager.h>
#include <libkgoogle/request.h>
#include <libkgoogle/reply.h>
#include <libkgoogle/auth.h>
#include <libkgoogle/objects/calendar.h>
#include <libkgoogle/objects/tasklist.h>
#include <libkgoogle/services/calendar.h>
#include <libkgoogle/services/tasks.h>
#include <libkgoogle/ui/accountscombo.h>

using namespace KGoogle;

enum {
  KGoogleObjectRole = Qt::UserRole,
  ObjectUIDRole = Qt::UserRole + 1
};

SettingsDialog::SettingsDialog( WId windowId, QWidget *parent ):
  KDialog( parent ),
  m_windowId( windowId )
{
  qRegisterMetaType<KGoogle::Services::Calendar>( "Calendar" );
  qRegisterMetaType<KGoogle::Services::Tasks>( "Tasks" );

  KWindowSystem::setMainWindow( this, windowId );

  m_ui = new ::Ui::SettingsDialog();
  m_ui->setupUi( this->mainWidget() );

  m_ui->addAccountBtn->setIcon( QIcon::fromTheme( "list-add-user" ) );
  m_ui->removeAccountBtn->setIcon( QIcon::fromTheme( "list-remove-user" ) );
  m_ui->reloadCalendarsBtn->setIcon( QIcon::fromTheme( "view-refresh" ) );
  m_ui->addCalBtn->setIcon( QIcon::fromTheme( "list-add" ) );
  m_ui->removeCalBtn->setIcon( QIcon::fromTheme( "list-remove" ) );
  m_ui->reloadTasksBtn->setIcon( QIcon::fromTheme( "view-refresh" ) );
  m_ui->addTasksBtn->setIcon( QIcon::fromTheme( "list-add" ) );
  m_ui->removeTasksBtn->setIcon( QIcon::fromTheme( "list-remove" ) );

  connect( m_ui->addAccountBtn, SIGNAL(clicked()),
           this, SLOT(addAccountClicked()) );
  connect( m_ui->removeAccountBtn, SIGNAL(clicked()),
           this, SLOT(removeAccountClicked()) );
  connect( m_ui->accountsCombo, SIGNAL(currentIndexChanged(int)),
           this, SLOT(accountChanged()) );
  connect( m_ui->addCalBtn, SIGNAL(clicked()),
           this, SLOT(addCalendarClicked()) );
  connect( m_ui->editCalBtn, SIGNAL(clicked()),
           this, SLOT(editCalendarClicked()) );
  connect( m_ui->calendarsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
           this, SLOT(editCalendarClicked()) );
  connect( m_ui->removeCalBtn, SIGNAL(clicked()),
           this, SLOT(removeCalendarClicked()) );
  connect( m_ui->reloadCalendarsBtn, SIGNAL(clicked()),
           this, SLOT(reloadCalendarsClicked()) );
  connect( m_ui->addTasksBtn, SIGNAL(clicked()),
           this, SLOT(addTaskListClicked()) );
  connect( m_ui->editTasksBtn, SIGNAL(clicked()),
           this, SLOT(editTaskListClicked()) );
  connect( m_ui->tasksList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
           this, SLOT(editTaskListClicked()) );
  connect( m_ui->removeTasksBtn, SIGNAL(clicked()),
           this, SLOT(removeTaskListClicked()) );
  connect( m_ui->reloadTasksBtn, SIGNAL(clicked()),
           this, SLOT(reloadTaskListsClicked()) );

  connect( this, SIGNAL(accepted()),
           this, SLOT(saveSettings()) );

  m_gam = new KGoogle::AccessManager;
  connect( m_gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(gam_objectsListReceived(KGoogle::Reply*)) );
  connect( m_gam, SIGNAL(error(KGoogle::Error,QString)),
           this, SLOT(error(KGoogle::Error,QString)) );

  KGoogle::Auth *auth = KGoogle::Auth::instance();
  connect( auth, SIGNAL(authenticated(KGoogle::Account::Ptr&)),
           this, SLOT(reloadAccounts()) );

  m_ui->accountsCombo->clear();
  reloadAccounts();
}

SettingsDialog::~SettingsDialog()
{
  Settings::self()->writeConfig();

  delete m_gam;

  delete m_ui;
}

void SettingsDialog::error( KGoogle::Error code, const QString &msg )
{
  KMessageBox::sorry( this, msg, i18n( "Error while talking to Google" ) );

  m_ui->accountsBox->setEnabled( true );
  m_ui->calendarsBox->setEnabled( true );
  m_ui->tasksBox->setEnabled( true );

  Q_UNUSED( code )
}

void SettingsDialog::saveSettings()
{
  Settings::self()->setAccount( m_ui->accountsCombo->currentText() );

  QStringList calendars;
  for ( int i = 0; i < m_ui->calendarsList->count(); i++ ) {
    QListWidgetItem *item = m_ui->calendarsList->item( i );

    if ( item->checkState() == Qt::Checked ) {
      calendars.append( item->data( ObjectUIDRole ).toString() );
    }
  }
  Settings::self()->setCalendars( calendars );

  QStringList taskLists;
  for ( int i = 0; i < m_ui->tasksList->count(); i++ ) {
    QListWidgetItem *item = m_ui->tasksList->item( i );

    if ( item->checkState() == Qt::Checked ) {
      taskLists.append( item->data( ObjectUIDRole ).toString() );
    }
  }
  Settings::self()->setTaskLists( taskLists );

  Settings::self()->writeConfig();
}

void SettingsDialog::reloadAccounts()
{
  disconnect( m_ui->accountsCombo, SIGNAL(currentIndexChanged(int)),
              this, SLOT(accountChanged()) );

  int accCnt = m_ui->accountsCombo->count();
  m_ui->accountsCombo->reload();

  QString accName = Settings::self()->account();
  int index = m_ui->accountsCombo->findText( accName );
  if ( index > -1 ) {
    m_ui->accountsCombo->setCurrentIndex( index );
  }

  if ( accCnt != m_ui->accountsCombo->count() ) {
    accountChanged();
  }

  connect( m_ui->accountsCombo, SIGNAL(currentIndexChanged(int)),
           this, SLOT(accountChanged()) );
}

void SettingsDialog::addAccountClicked()
{
  KGoogle::Auth *auth = KGoogle::Auth::instance();

  KGoogle::Account::Ptr account( new KGoogle::Account() );
  account->addScope( Services::Calendar::ScopeUrl );
  account->addScope( Services::Tasks::ScopeUrl );

  try {
    auth->authenticate( account, true );
  } catch ( KGoogle::Exception::BaseException &e ) {
    KMessageBox::error( this, e.what() );
  }
}

void SettingsDialog::removeAccountClicked()
{
  KGoogle::Account::Ptr account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  if ( KMessageBox::warningYesNo(
        this,
        i18n( "Do you really want to revoke access to account <b>%1</b>?"
              "<p>This will revoke access to all resources using this account!</p>",
              account->accountName() ),
        i18n( "Revoke Access?" ),
        KStandardGuiItem::yes(),
        KStandardGuiItem::no(),
        QString(),
        KMessageBox::Dangerous ) != KMessageBox::Yes ) {

    return;
  }

  KGoogle::Auth *auth = KGoogle::Auth::instance();
  try {
    auth->revoke( account );
  } catch ( KGoogle::Exception::BaseException &e ) {
    KMessageBox::error( this, e.what() );
  }

  reloadAccounts();
}

void SettingsDialog::accountChanged()
{
  m_ui->accountsBox->setDisabled( true );
  m_ui->calendarsBox->setDisabled( true );
  m_ui->tasksBox->setDisabled( true );

  Account::Ptr account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    m_ui->accountsBox->setEnabled( true );
    m_ui->calendarsList->clear();
    m_ui->calendarsBox->setEnabled( true );
    m_ui->tasksList->clear();
    m_ui->tasksBox->setEnabled( true );
    return;
  }

  KGoogle::Request *request;

  m_ui->calendarsList->clear();
  request = new KGoogle::Request( Services::Calendar::fetchCalendarsUrl(),
                                  Request::FetchAll, "Calendar", account );
  m_gam->queueRequest( request );

  m_ui->tasksList->clear();
  request = new KGoogle::Request( Services::Tasks::fetchTaskListsUrl(),
                                  Request::FetchAll, "Tasks", account );
  m_gam->sendRequest( request );
}

void SettingsDialog::addCalendarClicked()
{
  CalendarEditor *editor = new CalendarEditor;
  connect( editor, SIGNAL(accepted(KGoogle::Objects::Calendar*)),
           this, SLOT(addCalendar(KGoogle::Objects::Calendar*)) );

  editor->exec();

  delete editor;
}

void SettingsDialog::addCalendar( KGoogle::Objects::Calendar *calendar )
{
  KGoogle::Account::Ptr account;
  KGoogle::AccessManager *gam;
  KGoogle::Request *request;
  Services::Calendar parser;
  QByteArray data;

  account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->calendarsBox->setDisabled( true );

  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(gam_objectCreated(KGoogle::Reply*)) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Calendar::createCalendarUrl(),
                                  Request::Create, "Calendar", account );
  data = parser.objectToJSON( dynamic_cast< KGoogle::Object * >( calendar ) );
  request->setRequestData( data, "application/json" );
  gam->sendRequest( request );

  delete calendar;
}

void SettingsDialog::editCalendarClicked()
{
  Objects::Calendar *calendar;
  QListWidgetItem *item;
  QList< QListWidgetItem * > selected;

  selected = m_ui->calendarsList->selectedItems();
  if ( selected.isEmpty() ) {
    return;
  }

  item = selected.first();
  if ( !item ) {
    return;
  }

  calendar = item->data( KGoogleObjectRole ).value< KGoogle::Objects::Calendar * >();
  if ( !calendar ) {
    return;
  }

  CalendarEditor *editor = new CalendarEditor( calendar );
  connect( editor, SIGNAL(accepted(KGoogle::Objects::Calendar*)),
           this, SLOT(editCalendar(KGoogle::Objects::Calendar*)) );

  editor->exec();

  delete editor;
}

void SettingsDialog::editCalendar( KGoogle::Objects::Calendar *calendar )
{
  KGoogle::Account::Ptr account;
  KGoogle::AccessManager *gam;
  KGoogle::Request *request;
  Services::Calendar parser;
  QByteArray data;

  account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->calendarsBox->setDisabled( true );

  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(gam_objectModified(KGoogle::Reply*)) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Calendar::updateCalendarUrl( calendar->uid() ),
                                  Request::Update, "Calendar", account );
  data = parser.objectToJSON( dynamic_cast< KGoogle::Object * >( calendar ) );
  request->setRequestData( data, "application/json" );
  gam->sendRequest( request );

  delete calendar;
}

void SettingsDialog::removeCalendarClicked()
{
  Objects::Calendar *calendar;
  QListWidgetItem *item;
  QList< QListWidgetItem * > selected;

  selected = m_ui->calendarsList->selectedItems();
  if ( selected.isEmpty() ) {
    return;
  }

  item = selected.first();
  if ( !item ) {
    return;
  }

  calendar = item->data( KGoogleObjectRole ).value< KGoogle::Objects::Calendar * >();
  if ( !calendar ) {
    return;
  }

  if ( KMessageBox::warningYesNo(
         this,
         i18n( "Do you really want to remove calendar <b>%1</b>?"
               "<p>This will remove the calendar from Google servers as well!</p>",
               calendar->title() ),
         i18n( "Remove Calendar?" ),
         KStandardGuiItem::yes(),
         KStandardGuiItem::no(),
         QString(),
         KMessageBox::Dangerous ) != KStandardGuiItem::Yes ) {

    return;
  }

  KGoogle::Account::Ptr account;
  KGoogle::AccessManager *gam;
  KGoogle::Request *request;

  account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->calendarsBox->setDisabled( true );

  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(reloadCalendarsClicked()) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Calendar::removeCalendarUrl( calendar->uid() ),
                                  Request::Remove, "Calendar", account );
  gam->sendRequest( request );
}

void SettingsDialog::addTaskListClicked()
{
  TasklistEditor *editor = new TasklistEditor;
  connect( editor, SIGNAL(accepted(KGoogle::Objects::TaskList*)),
           this, SLOT(addTaskList(KGoogle::Objects::TaskList*)) );

  editor->exec();

  delete editor;
}

void SettingsDialog::reloadCalendarsClicked()
{
  KGoogle::AccessManager *gam;
  KGoogle::Account::Ptr account;
  KGoogle::Request *request;

  account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->calendarsBox->setDisabled( true );

  m_ui->calendarsList->clear();
  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(gam_objectsListReceived(KGoogle::Reply*)) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Calendar::fetchCalendarsUrl(),
                                  Request::FetchAll, "Calendar", account );
  gam->sendRequest( request );
}

void SettingsDialog::addTaskList( TaskList *taskList )
{
  KGoogle::Account::Ptr account;
  KGoogle::AccessManager *gam;
  KGoogle::Request *request;
  Services::Tasks parser;
  QByteArray data;

  account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->tasksBox->setDisabled( true );

  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(gam_objectCreated(KGoogle::Reply*)) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Tasks::createTaskListUrl(),
                                  Request::Create, "Tasks", account );
  data = parser.objectToJSON( dynamic_cast< KGoogle::Object * >( taskList ) );
  request->setRequestData( data, "application/json" );
  gam->sendRequest( request );

  delete taskList;
}

void SettingsDialog::editTaskListClicked()
{
  Objects::TaskList *taskList;
  QListWidgetItem *item;
  QList< QListWidgetItem * > selected;

  selected = m_ui->tasksList->selectedItems();
  if ( selected.isEmpty() ) {
    return;
  }

  item = selected.first();
  if ( !item ) {
    return;
  }

  taskList = item->data( KGoogleObjectRole ).value< KGoogle::Objects::TaskList * >();
  if ( !taskList ) {
    return;
  }

  TasklistEditor *editor = new TasklistEditor( taskList );
  connect( editor, SIGNAL(accepted(KGoogle::Objects::TaskList*)),
           this, SLOT(editTaskList(KGoogle::Objects::TaskList*)) );

  editor->exec();

  delete editor;
}

void SettingsDialog::editTaskList( TaskList *taskList )
{
  KGoogle::Account::Ptr account;
  KGoogle::AccessManager *gam;
  KGoogle::Request *request;
  Services::Tasks parser;
  QByteArray data;

  account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->tasksBox->setDisabled( true );

  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(gam_objectModified(KGoogle::Reply*)) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Tasks::updateTaskListUrl( taskList->uid() ),
                                  Request::Update, "Tasks", account );
  data = parser.objectToJSON( dynamic_cast< KGoogle::Object * >( taskList ) );
  request->setRequestData( data, "application/json" );
  gam->sendRequest( request );

  delete taskList;
}

void SettingsDialog::removeTaskListClicked()
{
  Objects::TaskList *taskList;
  QListWidgetItem *item;
  QList< QListWidgetItem * > selected;

  selected = m_ui->calendarsList->selectedItems();
  if ( selected.isEmpty() ) {
    return;
  }

  item = selected.first();
  if ( !item ) {
    return;
  }

  taskList = item->data( KGoogleObjectRole ).value< KGoogle::Objects::TaskList * >();
  if ( !taskList ) {
    return;
  }

  if ( KMessageBox::warningYesNo(
         this,
         i18n( "Do you really want to remove tasklist <b>%1</b>?"
               "<p>This will remove the tasklist from Google servers as well!</p>",
               taskList->title() ),
         i18n( "Remove tasklist?" ),
         KStandardGuiItem::yes(),
         KStandardGuiItem::no(),
         QString(),
         KMessageBox::Dangerous ) != KStandardGuiItem::Yes ) {

    return;
  }

  KGoogle::Account::Ptr account;
  KGoogle::AccessManager *gam;
  KGoogle::Request *request;

  account = m_ui->accountsCombo->currentAccount();
  if ( account.isNull() ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->tasksBox->setDisabled( true );

  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(reloadTaskListsClicked()) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Tasks::removeTaskListUrl( taskList->uid() ),
                                  Request::Remove, "Tasks", account );
  gam->sendRequest( request );
}

void SettingsDialog::reloadTaskListsClicked()
{
  KGoogle::AccessManager *gam;
  KGoogle::Account::Ptr account;
  KGoogle::Request *request;

  account = m_ui->accountsCombo->currentAccount();
  if ( !account ) {
    return;
  }

  m_ui->accountsBox->setDisabled( true );
  m_ui->tasksBox->setDisabled( true );

  m_ui->tasksList->clear();

  gam = new KGoogle::AccessManager;
  connect( gam, SIGNAL(replyReceived(KGoogle::Reply*)),
           this, SLOT(gam_objectsListReceived(KGoogle::Reply*)) );
  connect( gam, SIGNAL(requestFinished(KGoogle::Request*)),
           gam, SLOT(deleteLater()) );

  request = new KGoogle::Request( Services::Tasks::fetchTaskListsUrl(),
                                  Request::FetchAll, "Tasks", account );
  gam->sendRequest( request );
}

void SettingsDialog::gam_objectCreated( Reply *reply )
{
  QList< KGoogle::Object * > objects = reply->replyData();

  if ( reply->serviceName() == "Calendar" ) {

    Q_FOREACH ( KGoogle::Object * object, objects ) {
      KGoogle::Objects::Calendar *calendar = static_cast< KGoogle::Objects::Calendar * >( object );

      QListWidgetItem *item = new QListWidgetItem( calendar->title() );
      item->setData( KGoogleObjectRole, qVariantFromValue( calendar ) );
      item->setData( ObjectUIDRole, calendar->uid() );
      item->setCheckState( Qt::Unchecked );

      m_ui->calendarsList->addItem( item );
    }

    m_ui->calendarsBox->setEnabled( true );
    m_ui->accountsBox->setEnabled( true );

  } else if ( reply->serviceName() == "Tasks" ) {

    Q_FOREACH ( KGoogle::Object * object, objects ) {
      KGoogle::Objects::TaskList *taskList = static_cast< KGoogle::Objects::TaskList * >( object );

      QListWidgetItem *item = new QListWidgetItem( taskList->title() );
      item->setData( KGoogleObjectRole, qVariantFromValue( taskList ) );
      item->setData( ObjectUIDRole, taskList->uid() );
      item->setCheckState( Qt::Unchecked );

      m_ui->tasksList->addItem( item );
    }

    m_ui->tasksBox->setEnabled( true );
    m_ui->accountsBox->setEnabled( true );

  }

  delete reply;
}

void SettingsDialog::gam_objectsListReceived( Reply *reply )
{
  QList< KGoogle::Object * > objects = reply->replyData();

  if ( reply->serviceName() == "Calendar" ) {

    Q_FOREACH ( KGoogle::Object * object, objects ) {
      Objects::Calendar *calendar;
      QListWidgetItem *item;

      calendar = static_cast< Objects::Calendar * >( object );
      item = new QListWidgetItem;
      item->setText( calendar->title() );
      item->setData( KGoogleObjectRole, qVariantFromValue( calendar ) );
      item->setData( ObjectUIDRole, calendar->uid() );

      if ( Settings::self()->calendars().contains( calendar->uid() ) ) {
        item->setCheckState( Qt::Checked );
      } else {
        item->setCheckState( Qt::Unchecked );
      }

      m_ui->calendarsList->addItem( item );
    }

    m_ui->calendarsBox->setEnabled( true );
    delete reply;

  } else if ( reply->serviceName() == "Tasks" ) {

    Q_FOREACH ( KGoogle::Object *object, objects ) {
      Objects::TaskList *taskList;
      QListWidgetItem *item;

      taskList = static_cast< Objects::TaskList * >( object );
      item = new QListWidgetItem;
      item->setText( taskList->title() );
      item->setData( KGoogleObjectRole, qVariantFromValue( taskList ) );
      item->setData( ObjectUIDRole, taskList->uid() );

      if ( Settings::self()->taskLists().contains( taskList->uid() ) ) {
        item->setCheckState( Qt::Checked );
      } else {
        item->setCheckState( Qt::Unchecked );
      }

      m_ui->tasksList->addItem( item );
    }

    m_ui->tasksBox->setEnabled( true );
    delete reply;

  }

  if ( m_ui->calendarsBox->isEnabled() && m_ui->tasksList->isEnabled() ) {
    m_ui->accountsBox->setEnabled( true );
  }
}

void SettingsDialog::gam_objectModified( Reply *reply )
{
  QList< KGoogle::Object * > objects = reply->replyData();

  if ( reply->serviceName() == "Calendar" ) {

    Q_FOREACH ( KGoogle::Object * object, objects ) {
      KGoogle::Objects::Calendar *calendar = static_cast< KGoogle::Objects::Calendar * >( object );
      QListWidgetItem *item = 0;

      for ( int i = 0; i < m_ui->calendarsList->count(); i++ ) {
        QListWidgetItem *t = m_ui->calendarsList->item( i );

        if ( t->data( ObjectUIDRole ).toString() == calendar->uid() ) {
          item = t;
          break;
        }
      }

      if ( !item ) {
        item = new QListWidgetItem;
        m_ui->calendarsList->addItem( item );
      }

      item->setText( calendar->title() );
      item->setData( ObjectUIDRole, calendar->uid() );
      item->setData( KGoogleObjectRole, qVariantFromValue( calendar ) );
    }

    m_ui->calendarsBox->setEnabled( true );
    m_ui->accountsBox->setEnabled( true );

  } else if ( reply->serviceName() == "Tasks" ) {

    Q_FOREACH ( KGoogle::Object * object, objects ) {
      KGoogle::Objects::TaskList *taskList = static_cast< KGoogle::Objects::TaskList * >( object );
      QListWidgetItem *item = 0;

      for ( int i = 0; i < m_ui->tasksList->count(); i++ ) {
        QListWidgetItem *t = m_ui->tasksList->item( i );

        if ( t->data( ObjectUIDRole ).toString() == taskList->uid() ) {
          item = t;
          break;
        }
      }

      if ( !item ) {
        item = new QListWidgetItem;
        m_ui->tasksList->addItem( item );
      }

      item->setText( taskList->title() );
      item->setData( ObjectUIDRole, taskList->uid() );
      item->setData( KGoogleObjectRole, qVariantFromValue( taskList ) );

    }
    m_ui->tasksBox->setEnabled( true );
    m_ui->accountsBox->setEnabled( true );

  }

  delete reply;
}
