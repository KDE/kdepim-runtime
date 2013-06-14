/*
   Copyright (C) 2011-2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef GOOGLERESOURCE_H
#define GOOGLERESOURCE_H

#include <Akonadi/ResourceBase>
#include <Akonadi/AgentBase>

#include <QtGui/qwindowdefs.h>

#include <LibKGAPI2/Types>

#include <KDE/KLocalizedString>

#define ITEM_PROPERTY "_AkonadiItem"
#define ITEMLIST_PROPERTY "_AkonadiItemList"
#define COLLECTION_PROPERTY "_AkonadiCollection"
#define JOB_PROPERTY "_KGAPI2Job"

namespace KGAPI2
{
class Job;
}

class Settings;

class GoogleResource : public Akonadi::ResourceBase,
                       public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

  public:
    explicit GoogleResource( const QString &id );
    virtual ~GoogleResource();

    virtual Settings* settings() const = 0;

    KGAPI2::AccountPtr account() const;

  public Q_SLOTS:
    void reloadConfig();

    virtual void slotTokensReceived( KJob *job );

  protected Q_SLOTS:
    virtual bool retrieveItem( const Akonadi::Item &item, const QSet< QByteArray > &parts );

    bool handleError( KGAPI2::Job *job );

    virtual void slotGenericJobFinished( KGAPI2::Job *job );

    void emitPercent( KGAPI2::Job* job, int processedCount, int totalCount );

    virtual void slotAbortRequested();

    virtual void updateResourceName() = 0;

  protected:
    template <typename T>
    bool canPerformTask( const Akonadi::Item &item, const QString &mimeType = QString() ) {
        if ( item.isValid() && !item.hasPayload<T>()) {
            cancelTask( i18n( "Invalid item payload." ) );
            return false;
        } else if ( item.isValid() && mimeType != item.mimeType() ) {
            cancelTask( i18n( "Invalid payload mimetype. Expected %1, found %2", mimeType, item.mimeType() ) );
            return false;
        }

        return canPerformTask();
    }

    bool canPerformTask();


    virtual void aboutToQuit();

  private Q_SLOTS:
    void slotAccountInfoReceived( KGAPI2::Job *job );

  private:
    void finishAuthentication( KGAPI2::Job *job );
    void abort();

    KGAPI2::AccountPtr m_account;

};

#endif // GOOGLERESOURCE_H
