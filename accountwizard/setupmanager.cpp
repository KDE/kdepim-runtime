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
#include "configfile.h"
#include "ldap.h"
#include "identity.h"

#include <kemailsettings.h>
#include <kwallet.h>
#include <QLocale>

SetupManager::SetupManager(QWidget *parent) :
    QObject(parent),
    m_currentSetupObject(0),
    m_page(0),
    m_wallet(0),
    m_personalDataAvailable(false),
    m_rollbackRequested(false)
{
    KEMailSettings e;
    m_name = e.getSetting(KEMailSettings::RealName);
    m_email = e.getSetting(KEMailSettings::EmailAddress);
}

SetupManager::~SetupManager()
{
    delete m_wallet;
}

void SetupManager::setSetupPage(SetupPage *page)
{
    m_page = page;
}

QObject *SetupManager::createResource(const QString &type)
{
    return connectObject(new Resource(type, this));
}

QObject *SetupManager::createTransport(const QString &type)
{
    return connectObject(new Transport(type, this));
}

QObject *SetupManager::createConfigFile(const QString &fileName)
{
    return connectObject(new ConfigFile(fileName, this));
}

QObject *SetupManager::createLdap()
{
    return connectObject(new Ldap(this));
}

QObject *SetupManager::createIdentity()
{
    return connectObject(new Identity(this));
}

static bool dependencyCompare(SetupObject *left, SetupObject *right)
{
    if (!left->dependsOn() && right->dependsOn()) {
        return true;
    }
    return false;
}

void SetupManager::execute()
{
    m_page->setStatus(i18n("Setting up account..."));
    m_page->setValid(false);

    // ### FIXME this is a bad over-simplification and would need a real topological sort
    // but for current usage it is good enough
    qStableSort(m_objectToSetup.begin(), m_objectToSetup.end(), dependencyCompare);
    setupNext();
}

void SetupManager::setupSucceeded(const QString &msg)
{
    Q_ASSERT(m_page);
    m_page->addMessage(SetupPage::Success, msg);
    if (m_currentSetupObject) {
        m_setupObjects.append(m_currentSetupObject);
        m_currentSetupObject = 0;
    }
    setupNext();
}

void SetupManager::setupFailed(const QString &msg)
{
    Q_ASSERT(m_page);
    m_page->addMessage(SetupPage::Error, msg);
    if (m_currentSetupObject) {
        m_setupObjects.append(m_currentSetupObject);
        m_currentSetupObject = 0;
    }
    rollback();
}

void SetupManager::setupInfo(const QString &msg)
{
    Q_ASSERT(m_page);
    m_page->addMessage(SetupPage::Info, msg);
}

void SetupManager::setupNext()
{
    // user canceld during the previous setup step
    if (m_rollbackRequested) {
        rollback();
        return;
    }

    if (m_objectToSetup.isEmpty()) {
        m_page->setStatus(i18n("Setup complete."));
        m_page->setProgress(100);
        m_page->setValid(true);
    } else {
        const int setupObjectCount = m_objectToSetup.size() + m_setupObjects.size();
        const int remainingObjectCount = setupObjectCount - m_objectToSetup.size();
        m_page->setProgress((remainingObjectCount * 100) / setupObjectCount);
        m_currentSetupObject = m_objectToSetup.takeFirst();
        m_currentSetupObject->create();
    }
}

void SetupManager::rollback()
{
    m_page->setStatus(i18n("Failed to set up account, rolling back..."));
    const int setupObjectCount = m_objectToSetup.size() + m_setupObjects.size();
    int remainingObjectCount = m_setupObjects.size();
    foreach (SetupObject *obj, m_setupObjects) {
        m_page->setProgress((remainingObjectCount * 100) / setupObjectCount);
        if (obj) {
            obj->destroy();
            m_objectToSetup.prepend(obj);
        }
    }
    m_setupObjects.clear();
    m_page->setProgress(0);
    m_page->setStatus(i18n("Failed to set up account."));
    m_page->setValid(true);
    m_rollbackRequested = false;
    emit rollbackComplete();
}

SetupObject *SetupManager::connectObject(SetupObject *obj)
{
    connect(obj, &SetupObject::finished, this, &SetupManager::setupSucceeded);
    connect(obj, &SetupObject::info, this, &SetupManager::setupInfo);
    connect(obj, &SetupObject::error, this, &SetupManager::setupFailed);
    m_objectToSetup.append(obj);
    return obj;
}

void SetupManager::setName(const QString &name)
{
    m_name = name;
}

QString SetupManager::name()
{
    return m_name;
}

void SetupManager::setEmail(const QString &email)
{
    m_email = email;
}

QString SetupManager::email()
{
    return m_email;
}

void SetupManager::setPassword(const QString &password)
{
    m_password = password;
}

QString SetupManager::password()
{
    return m_password;
}

QString SetupManager::country()
{
    return QLocale::countryToString(QLocale().country());
}

void SetupManager::openWallet()
{
    using namespace KWallet;
    if (Wallet::isOpen(Wallet::NetworkWallet())) {
        return;
    }

    Q_ASSERT(parent()->isWidgetType());
    m_wallet = Wallet::openWallet(Wallet::NetworkWallet(), qobject_cast<QWidget *>(parent())->effectiveWinId(), Wallet::Asynchronous);
    QEventLoop loop;
    connect(m_wallet, &KWallet::Wallet::walletOpened, &loop, &QEventLoop::quit);
    loop.exec();
}

bool SetupManager::personalDataAvailable()
{
    return m_personalDataAvailable;
}

void SetupManager::setPersonalDataAvailable(bool available)
{
    m_personalDataAvailable = available;
}

void SetupManager::requestRollback()
{
    if (m_setupObjects.isEmpty()) {
        emit rollbackComplete();
    } else {
        m_rollbackRequested = true;
        if (!m_currentSetupObject) {
            rollback();
        }
    }
}

