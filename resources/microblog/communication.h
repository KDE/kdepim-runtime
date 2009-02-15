/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include <QObject>
#include <kio/job.h>

class Communication : public QObject
{
    Q_OBJECT

public:
    Communication( QObject *parent );
    ~Communication();
    enum Service { Identi, Twitter };
    void setService( int );
    void setCredentials( const QString&, const QString& );
    void checkAuth();
    void retrieveFolder( const QString&, qlonglong since=0 );

private:
    QString serviceToApi( int service );
    KUrl getBaseUrl();

    int m_service;
    QString m_username;
    QString m_password;
    QString m_retrievingFolder;

private slots:
    void slotCheckAuthData( KJob* );
    void slotStatusListReceived( KJob* job );

signals:
    void authOk();
    void authFailed( const QString& );
    void statusList( const QList<QByteArray> );
};


#endif
