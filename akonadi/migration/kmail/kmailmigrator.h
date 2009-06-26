/*
    Copyright (c) 2009 Jonathan Armond <jon.armond@gmail.com>

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

#ifndef KMAILMIGRATOR_H
#define KMAILMIGRATOR_H

#include "kmigratorbase.h"

#include <QObject>
#include <QStringList>

class KConfig;
class KJob;

namespace KMail
{
class AccountManager;
class ImapAccountBase;

/**
 * KMail to Akonadi account migrator
 */
class KMailMigrator : public KMigratorBase
{
  Q_OBJECT

  public:
    KMailMigrator( const QStringList &typesToMigrate );
    virtual ~KMailMigrator();

    void migrate();

    void migrateNext();

  private slots:
    void imapAccountCreated( KJob *job );
    //void pop3AccountCreated( KJob *job );
    void mboxAccountCreated( KJob *job );
    void maildirAccountCreated( KJob *job );

  private:
    bool migrateCurrentAccount();
    void migrationCompleted( const Akonadi::AgentInstance &instance );
    void migrationFailed( const QString &errorMsg, const Akonadi::AgentInstance &instance
                          = Akonadi::AgentInstance() );

  private:
    QStringList mTypes;
    KConfig *mConfig;
    QString mCurrentAccount;
    QStringList mAccounts;
    typedef QStringList::iterator AccountIterator;
    AccountIterator mIt;
};

} // namespace KMail

#endif
