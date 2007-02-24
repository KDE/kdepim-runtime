/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_RESOURCEBASE_H
#define AKONADI_RESOURCEBASE_H

#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QString>

#include <kdepim_export.h>

#include <libakonadi/job.h>

#include "resource.h"

class KJob;

namespace Akonadi {

class Job;
class Session;

/**
 * This class should be used as subclass by all resource agents
 * since it encapsulates large parts of the protocol between
 * resource agent, agent manager and storage.
 *
 * It provides many convenience methods to make implementing a
 * new akonadi resource agent as simple as possible.
 */
class AKONADI_RESOURCES_EXPORT ResourceBase : public Resource
{
  Q_OBJECT

  public:
    /**
     * This enum describes the different states the
     * resource can be in.
     */
    enum Status
    {
      Ready = 0,
      Syncing,
      Error
    };

    /**
     * Use this method in the main function of your resource
     * application to initialize your resource subclass.
     *
     * \code
     *
     *   class MyResource : public ResourceBase
     *   {
     *     ...
     *   };
     *
     *   int main( int argc, char **argv )
     *   {
     *     QCoreApplication app;
     *
     *     ResourceBase::init<MyResource>( argc, argv );
     *
     *     return app.exec();
     *   }
     *
     * \endcode
     */
    template <typename T>
    static void init( int argc = 0, char **argv = 0 )
    {
      QString id = parseArguments( argc, argv );
      new T( id );
    }

    /**
     * This method returns the current status code of the resource.
     *
     * The following return values are possible:
     *
     *  0 - Ready
     *  1 - Syncing
     *  2 - Error
     */
    virtual int status() const;

    /**
     * This method returns an i18n'ed description of the current status code.
     */
    virtual QString statusMessage() const;

    /**
     * This method returns the current progress of the resource in percentage.
     */
    virtual uint progress() const;

    /**
     * This method returns an i18n'ed description of the current progress.
     */
    virtual QString progressMessage() const;

    /**
     * This method is called whenever the resource shall show its configuration dialog
     * to the user. It will be automatically called when the resource is started for
     * the first time.
     */
    virtual void configure();

    /**
     * This method is called whenever the resource should start synchronization.
     */
    virtual void synchronize() = 0;

    /**
     * This method is used to set the name of the resource.
     */
    virtual void setName( const QString &name );

    /**
     * Returns the name of the resource.
     */
    virtual QString name() const;

    /**
     * Returns the instance identifier of this resource.
     */
    QString identifier() const;

    /**
     * This method is called when the resource is removed from
     * the system, so it can do some cleanup stuff.
     */
    virtual void cleanup() const;

    /**
     * This method is called from the crash handler, don't call
     * it manually.
     */
    void crashHandler( int signal );

  public Q_SLOTS:
    /**
     * This method is called to quit the resource.
     *
     * Before the application is terminated @see aboutToQuit() is called,
     * which can be reimplemented to do some session cleanup (e.g. disconnecting
     * from groupware server).
     */
    void quit();

    /**
      Enables change recording. When change recording is enabled all changes are
      stored internally and replayed as soon as change recording is disabled.
      @param enable True to enable change recoding, false to disable change recording.
    */
    void enableChangeRecording( bool enable );

  protected:
    /**
     * Creates a base resource.
     *
     * @param id The instance id of the resource.
     */
    ResourceBase( const QString & id );

    /**
     * Destroys the base resource.
     */
    ~ResourceBase();

    /**
     * This method shall be used to report warnings.
     */
    void warning( const QString& message );

    /**
     * This method shall be used to report errors.
     */
    void error( const QString& message );

    /**
     * This method shall be used to signal a state change.
     *
     * @param status The new status of the resource.
     * @param message An i18n'ed description of the status. If message
     *                is empty, the default description for the status is used.
     */
    void changeStatus( Status status, const QString &message = QString() );

    /**
     * This method shall be used to signal a progress change.
     *
     * @param progress The new progress of the resource in percentage.
     * @param message An i18n'ed description of the progress.
     */
    void changeProgress( uint progress, const QString &message = QString() );

    /**
     * This method is called whenever the application is about to
     * quit.
     *
     * Reimplement this method to do session cleanup (e.g. disconnecting
     * from groupware server).
     */
    virtual void aboutToQuit();

    /**
     * This method returns the settings object which has to be used by the
     * resource to store its configuration data.
     *
     * Don't delete this object!
     */
    QSettings* settings();

    /**
     * Returns a session for communicating with the storage backend. It should
     * be used for all jobs.
     */
    Session* session();

    /**
      Call this method from in requestItemDelivery(). It will generate an appropriate
      D-Bus reply as soon as the given job has finished.
      @param job The job which actually delivers the item.
      @param msg The D-Bus message requesting the delivery.
    */
    bool deliverItem( Akonadi::Job* job, const QDBusMessage &msg );

    /**
      Reimplement to handle adding of new items.
      @param ref DataReference to the newly added item.
    */
    virtual void itemAdded( const DataReference &ref ) { Q_UNUSED( ref ); }

    /**
      Reimplement to handle changes to existing items.
      @param ref DataReference to the changed item.
    */
    virtual void itemChanged( const DataReference &ref ) { Q_UNUSED( ref ); }

    /**
      Reimplement to handle deletion of items.
      @param ref DataReference to the deleted item.
    */
    virtual void itemRemoved( const DataReference &ref ) { Q_UNUSED( ref ); }

    /**
      Resets the dirty flag of the given item. Call whenever you
      have successfully written changes back to the server.
      @param ref DataReference of the item.
    */
    void changesCommitted( const DataReference &ref );

  private:
    static QString parseArguments( int, char** );

  private Q_SLOTS:
    void slotDeliveryDone( KJob* job );

    void slotItemAdded( const Akonadi::DataReference &ref );
    void slotItemChanged( const Akonadi::DataReference &ref );
    void slotItemRemoved( const Akonadi::DataReference &ref );

  private:
    class Private;
    Private* const d;
};

}

#endif
