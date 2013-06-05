/* This file is part of the KDE Project
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>
   Copyright (c) 2010-13 Vishesh Handa <me@vhanda.in>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "eventmonitor.h"
#include "indexscheduler.h"

#include <KDebug>
#include <KPassivePopup>
#include <KLocale>
#include <KStandardDirs>
#include <KConfigGroup>
#include <KIdleTime>

#include <Solid/PowerManagement>

// TODO: Make idle timeout configurable?
static int s_idleTimeout = 1000 * 60 * 2; // 2 min

Nepomuk2::EventMonitor::EventMonitor( QObject* parent )
    : QObject( parent )
{
    // monitor the powermanagement to not drain the battery
    connect( Solid::PowerManagement::notifier(), SIGNAL( appShouldConserveResourcesChanged( bool ) ),
             this, SLOT(slotPowerManagementStatusChanged(bool)) );

    // setup idle time
    KIdleTime* idleTime = KIdleTime::instance();
    connect( idleTime, SIGNAL(timeoutReached(int)), this, SLOT(slotIdleTimeoutReached()) );
    connect( idleTime, SIGNAL(resumingFromIdle()), this, SLOT(slotResumeFromIdle()) );

    m_isOnBattery = Solid::PowerManagement::appShouldConserveResources();
    m_isIdle = false;
    m_enabled = false;

    enable();

    /*
  KConfigGroup cfgGrp( componentData().config(), identifier() );
  KIdleTime::instance()->addIdleTimeout( 1000 * cfgGrp.readEntry( "IdleTimeout", 120 ) );
  disableIdleDetection( cfgGrp.readEntry( "DisableIdleDetection", true ) );
  */
}


Nepomuk2::EventMonitor::~EventMonitor()
{
}


void Nepomuk2::EventMonitor::slotPowerManagementStatusChanged( bool conserveResources )
{
    m_isOnBattery = conserveResources;
    if( m_enabled ) {
        emit powerManagementStatusChanged( conserveResources );
    }
}


void Nepomuk2::EventMonitor::enable()
{
    /* avoid add multiple idle timeout */
    if (!m_enabled) {
        m_enabled = true;
        KIdleTime::instance()->addIdleTimeout( s_idleTimeout );
    }
}

void Nepomuk2::EventMonitor::disable()
{
    if (m_enabled) {
        m_enabled = false;
        KIdleTime::instance()->removeAllIdleTimeouts();
    }
}

void Nepomuk2::EventMonitor::slotIdleTimeoutReached()
{
    if( m_enabled ) {
        m_isIdle = true;
        emit idleStatusChanged( true );
    }
    KIdleTime::instance()->catchNextResumeEvent();
}

void Nepomuk2::EventMonitor::slotResumeFromIdle()
{
    m_isIdle = false;
    if( m_enabled ) {
        emit idleStatusChanged( false );
    }
}

#include "eventmonitor.moc"
