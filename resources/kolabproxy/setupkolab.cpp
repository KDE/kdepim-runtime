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

#include "setupkolab.h"

#include "kolabproxyresource.h"
#include "setupdefaultfoldersjob.h"
#include "upgradejob.h"

#include <Akonadi/AgentManager>

#include <KMessageBox>
#include <KStandardDirs>

#include <QProcess>

#define IMAP_RESOURCE_IDENTIFIER "akonadi_imap_resource"

SetupKolab::SetupKolab( KolabProxyResource *parentResource, WId parent )
  :  KDialog( QWidget::find( parent ) ),
     m_ui( new Ui::SetupKolabView ),
     m_versionUi( new Ui::ChangeFormatView ),
     m_parentResource( parentResource )
{
  Q_UNUSED( parent );
  m_ui->setupUi( mainWidget() );
  setButtons( Close );
  initConnection();
  updateCombobox();

  KConfigGroup grp( KGlobal::mainComponent().config(), "KolabProxyResourceSettings" );
  if ( !grp.readEntry( "enableKolabV3", false ) ) {
    m_ui->upgradeLabel->hide();
    m_ui->upgradeFormatButton->hide();
    grp.writeEntry( "enableKolabV3", false );
    grp.sync();
  }
}

SetupKolab::~SetupKolab()
{
  delete m_ui;
}

void SetupKolab::initConnection()
{
  connect( m_ui->launchWizard, SIGNAL(clicked()), this, SLOT(slotLaunchWizard()) );
  connect( m_ui->createKolabFolderButton, SIGNAL(clicked()),
           this, SLOT(slotCreateDefaultKolabCollections()) );
  connect( m_ui->upgradeFormatButton, SIGNAL(clicked()),
           this, SLOT(slotShowUpgradeDialog()) );
  connect( m_ui->imapAccountComboBox, SIGNAL(currentIndexChanged(QString)),
           this, SLOT(slotSelectedAccountChanged()) );
  connect( Akonadi::AgentManager::self(), SIGNAL(instanceAdded(Akonadi::AgentInstance)),
           this, SLOT(slotInstanceAddedRemoved()) );
  connect( Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)),
           this, SLOT(slotInstanceAddedRemoved()) );
}

void SetupKolab::slotShowUpgradeDialog()
{
  const Akonadi::AgentInstance instanceSelected =
    m_agentList[m_ui->imapAccountComboBox->currentText()];

  KDialog *dialog = new KDialog( this );
  dialog->setButtons( Close );
  m_versionUi->setupUi( dialog->mainWidget() );
  m_versionUi->progressBar->setDisabled( true );
  connect( m_versionUi->pushButton, SIGNAL(clicked()), this, SLOT(slotDoUpgrade()) );

  KConfigGroup grp( KGlobal::mainComponent().config(), "KolabProxyResourceSettings" );
  Kolab::Version v =
    static_cast<Kolab::Version>(
      grp.readEntry( "KolabFormatVersion" + instanceSelected.identifier(),
                     static_cast<int>( Kolab::KolabV2 ) ) );

  m_versionUi->formatVersion->insertItem( 0, "Kolab Format v2", Kolab::KolabV2 );
  m_versionUi->formatVersion->insertItem( 1, "Kolab Format v3", Kolab::KolabV3 );
  if ( v == Kolab::KolabV2 ) {
    m_versionUi->formatVersion->setCurrentIndex(0);
  } else {
    m_versionUi->formatVersion->setCurrentIndex(1);
  }
  dialog->exec();
  grp.writeEntry(
    "KolabFormatVersion" + instanceSelected.identifier(),
    m_versionUi->formatVersion->itemData( m_versionUi->formatVersion->currentIndex() ) );
  grp.sync();
  slotSelectedAccountChanged();
  dialog->deleteLater();
}

void SetupKolab::slotDoUpgrade()
{
  const Akonadi::AgentInstance instanceSelected =
    m_agentList[m_ui->imapAccountComboBox->currentText()];

  m_versionUi->statusLabel->setText( "started" );
  m_versionUi->progressBar->setEnabled( true );

  UpgradeJob *job =
    new UpgradeJob(
      static_cast<Kolab::Version>(
        m_versionUi->formatVersion->itemData(
          m_versionUi->formatVersion->currentIndex() ).toInt() ),
      instanceSelected, this );

  connect( job, SIGNAL(percent(KJob*,ulong)),
           this, SLOT(slotUpgradeProgress(KJob*,ulong)) );
  connect( job, SIGNAL(result(KJob*)),
           this, SLOT(slotUpgradeDone(KJob*)) );
}

void SetupKolab::slotUpgradeProgress( KJob *job, ulong value )
{
  Q_UNUSED( job );
  m_versionUi->progressBar->setValue( value );
}

void SetupKolab::slotUpgradeDone( KJob *job )
{
  if ( job->error() ) {
    kWarning() << job->errorString();
    m_versionUi->statusLabel->setText( "error" );
    KMessageBox::error(
      this,
      i18n( "Could not complete the upgrade process: " ) + job->errorString(),
      i18n( "Error during Upgrade Process" ) );
    return;
  }
  m_versionUi->statusLabel->setText( "complete" );
  m_versionUi->progressBar->setValue( 100 );
}

void SetupKolab::slotSelectedAccountChanged()
{
  const Akonadi::AgentInstance instanceSelected =
    m_agentList[m_ui->imapAccountComboBox->currentText()];

  KConfigGroup grp( KGlobal::mainComponent().config(), "KolabProxyResourceSettings" );
  Kolab::Version v =
    static_cast<Kolab::Version>(
      grp.readEntry( "KolabFormatVersion" + instanceSelected.identifier(),
                     static_cast<int>( Kolab::KolabV2 ) ) );

  if ( v == Kolab::KolabV2 ) {
    m_ui->formatVersion->setText( "Kolab Format v2" );
  } else {
    m_ui->formatVersion->setText( "Kolab Format v3" );
  }
}

void SetupKolab::updateCombobox()
{
  bool imapAccountFound = false;
  m_ui->imapAccountComboBox->clear();
  m_agentList.clear();

  Akonadi::AgentInstance::List relevantInstances;
  foreach ( const Akonadi::AgentInstance &instance, Akonadi::AgentManager::self()->instances() ) {
    if ( instance.identifier().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
      const QString instanceName = instance.name();
      m_agentList.insert( instanceName, instance );
      m_ui->imapAccountComboBox->addItem( instanceName );
      imapAccountFound = true;
    }
  }
  if ( imapAccountFound ) {
    m_ui->stackedWidget->setCurrentIndex( 1 );
  } else {
    m_ui->stackedWidget->setCurrentIndex( 0 );
  }
}

void SetupKolab::slotLaunchWizard()
{
  QStringList lst;
  lst.append( "--assistant" );
  lst.append( "imap" );

  const QString path = KStandardDirs::findExe( QLatin1String( "accountwizard" ) );
  if ( !QProcess::startDetached( path, lst ) ) {
    KMessageBox::error(
      this,
      i18n( "Could not start the account wizard. "
            "Please check your installation." ),
      i18n( "Unable to start account wizard" ) );
  }
}

void SetupKolab::slotInstanceAddedRemoved()
{
  updateCombobox();
}

void SetupKolab::slotCreateDefaultKolabCollections()
{
  const Akonadi::AgentInstance instanceSelected =
    m_agentList[m_ui->imapAccountComboBox->currentText()];

  if ( instanceSelected.isValid() ) {
    new SetupDefaultFoldersJob( instanceSelected, this );
  }
}

#include "setupkolab.moc"
