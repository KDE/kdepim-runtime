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

#include "setupmanager.h"
#include "resource.h"
#include "setuppage.h"
#include "transport.h"

SetupManager::SetupManager( QObject* parent) :
  QObject(parent),
  m_page( 0 )
{
}

void SetupManager::setSetupPage(SetupPage* page)
{
  m_page = page;
}

QObject* SetupManager::createResource(const QString& type)
{
  Resource *res = new Resource( type, this );
  connect( res, SIGNAL(finished(QString)), SLOT(setupSucceeded(QString)) );
  connect( res, SIGNAL(info(QString)), SLOT(setupInfo(QString)) );
  connect( res, SIGNAL(error(QString)), SLOT(setupFailed(QString)) );
  m_objectToSetup.append( res );
  return res;
}

QObject* SetupManager::createTransport(const QString& type)
{
  Transport *t = new Transport( type, this );
  connect( t, SIGNAL(finished(QString)), SLOT(setupSucceeded(QString)) );
  connect( t, SIGNAL(info(QString)), SLOT(setupInfo(QString)) );
  connect( t, SIGNAL(error(QString)), SLOT(setupFailed(QString)) );
  m_objectToSetup.append( t );
  return t;
}

void SetupManager::execute()
{
  m_page->setStatus( i18n( "Setting up account..." ) );
  m_page->setValid( false );
  setupNext();
}

void SetupManager::setupSucceeded(const QString& msg)
{
  Q_ASSERT( m_page );
  m_page->addMessage( SetupPage::Success, msg );
  m_setupObjects.append( m_currentSetupObject );
  m_currentSetupObject = 0;
  setupNext();
}

void SetupManager::setupFailed(const QString& msg)
{
  Q_ASSERT( m_page );
  m_page->addMessage( SetupPage::Error, msg );
  m_setupObjects.append( m_currentSetupObject );
  m_currentSetupObject = 0;
  rollback();
}

void SetupManager::setupInfo(const QString& msg)
{
  Q_ASSERT( m_page );
  m_page->addMessage( SetupPage::Info, msg );
}

void SetupManager::setupNext()
{
  if ( m_objectToSetup.isEmpty() ) {
    m_page->setStatus( i18n( "Setup complete." ) );
    m_page->setProgress( 100 );
    m_page->setValid( false );
  } else {
    const int setupObjectCount = m_objectToSetup.size() + m_setupObjects.size();
    const int remainingObjectCount = setupObjectCount - m_objectToSetup.size();
    m_page->setProgress( (remainingObjectCount * 100) / setupObjectCount );
    m_currentSetupObject = m_objectToSetup.takeFirst();
    m_currentSetupObject->create();
  }
}

void SetupManager::rollback()
{
  m_page->setStatus( i18n( "Failed to set up account, rolling back..." ) );
  const int setupObjectCount = m_objectToSetup.size() + m_setupObjects.size();
  int remainingObjectCount = m_setupObjects.size();
  foreach ( SetupObject* obj, m_setupObjects ) {
    m_page->setProgress( (remainingObjectCount * 100) / setupObjectCount );
    obj->destroy();
    m_objectToSetup.prepend( obj );
  }
  m_setupObjects.clear();
  m_page->setProgress( 0 );
  m_page->setStatus( i18n( "Failed to set up account." ) );
  m_page->setValid( true );
}

#include "setupmanager.moc"
