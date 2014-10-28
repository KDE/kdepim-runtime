/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef IMAPRESOURCE_H
#define IMAPRESOURCE_H

#include <imapresourcebase.h>

class ImapResource : public ImapResourceBase
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.Imap.Resource" )

public:
    explicit ImapResource( const QString &id );
    virtual ~ImapResource();

    virtual KDialog *createConfigureDialog ( WId windowId );
    virtual void cleanup();

protected:
    virtual QString defaultName() const;

private Q_SLOTS:
    void onConfigurationDone( int result );

};

#endif // IMAPRESOURCE_H
