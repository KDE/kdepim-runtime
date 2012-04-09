/*
    Copyright (c) 2010 Laurent Montel <montel@kde.org>

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
#include <KJob>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <KDE/KDebug>
#include <QProcess>
#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/entitydisplayattribute.h>
#include "collectionannotationsattribute.h"
#include "setupdefaultfoldersjob.h"
#include "settings.h"

#define IMAP_RESOURCE_IDENTIFIER "akonadi_imap_resource"

#define KOLAB_FOLDERTYPE "/vendor/kolab/folder-type"

SetupKolab::SetupKolab( KolabProxyResource* parentResource,WId parent )
:KConfigDialog(QWidget::find(parent), "settings", Settings::self() ),
   m_ui(new Ui::SetupKolabView),
   m_parentResource( parentResource )
{
  Q_UNUSED( parent );
  m_ui->setupUi( mainWidget() );
  initConnection();
  updateCombobox();
  setButtons( Close );
}

SetupKolab::~SetupKolab()
{
  delete m_ui;
}

void SetupKolab::initConnection()
{

  connect( m_ui->launchWizard, SIGNAL(clicked()), this, SLOT(slotLaunchWizard()) );
  connect( m_ui->createKolabFolderButton, SIGNAL(clicked()), this, SLOT(slotCreateDefaultKolabCollections()) );
  connect( Akonadi::AgentManager::self(), SIGNAL(instanceAdded(Akonadi::AgentInstance)), this, SLOT(slotInstanceAddedRemoved()) );
  connect( Akonadi::AgentManager::self(), SIGNAL(instanceRemoved(Akonadi::AgentInstance)), this, SLOT(slotInstanceAddedRemoved()) );

}

void SetupKolab::updateSettings()
{
    kDebug() << Settings::self()->formatVersion();
    KConfigDialog::updateSettings();
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

  const QString path = KStandardDirs::findExe( QLatin1String("accountwizard" ) );
  if( !QProcess::startDetached( path, lst ) )
    KMessageBox::error( this, i18n( "Could not start the account wizard. "
                                 "Please check your installation." ),
                        i18n( "Unable to start account wizard" ) );


}

void SetupKolab::slotInstanceAddedRemoved()
{
  updateCombobox();
}

void SetupKolab::slotCreateDefaultKolabCollections()
{
  const Akonadi::AgentInstance instanceSelected = m_agentList[m_ui->imapAccountComboBox->currentText()];
  if ( instanceSelected.isValid() ) {
    new SetupDefaultFoldersJob( instanceSelected, this );
#if 0
    Akonadi::Collection collection;
    collection.setName( "Calendar" );
    collection.setParentCollection( Akonadi::Collection::root() );
    qDebug()<<" instanceSelected.identifier() :"<<instanceSelected.identifier();
    collection.setResource( instanceSelected.identifier() );
    collection.setContentMimeTypes( QStringList() << Akonadi::Collection::mimeType() << QLatin1String( "message/rfc822" ) );
    Akonadi::CollectionAnnotationsAttribute *annotationsAttribute = collection.attribute<Akonadi::CollectionAnnotationsAttribute>( Akonadi::Entity::AddIfMissing );
    QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();

    annotations[ KOLAB_FOLDERTYPE ] = "task";
    createKolabCollection( collection );
#endif
  }
}

void SetupKolab::createKolabCollection( Akonadi::Collection & collection )
{
  Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( collection );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(createResult(KJob*)) );

}

void SetupKolab::createResult( KJob *job )
{
  if ( job->error() ) {
    kWarning()<<" Error during kolab collection created : "<<job->errorString();
    return;
  }
  Akonadi::CollectionCreateJob *createJob = static_cast<Akonadi::CollectionCreateJob*>( job );
  Akonadi::Collection collection = createJob->collection();
  bool collectionRegistered = m_parentResource->registerHandlerForCollection( collection );
  if ( !collectionRegistered ) {
    kWarning()<<" Can not register Collection "<<collection.id();
  }
}

#include "setupkolab.moc"
