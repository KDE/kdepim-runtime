/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#ifndef PIM_RESOURCEBASE_H
#define PIM_RESOURCEBASE_H

#include <QObject>
#include <QString>

#include <kdepim_export.h>

#include "resource.h"

namespace PIM {

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
    static void init( int argc, char **argv )
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
     * This method is called whenever the resource shall show its configuration dialog
     * to the user.
     */
    virtual void configure();

    /**
     * This method is used to set the configuration of the resource explicitely.
     *
     * @param data The configuration in xml format.
     * @returns true if the configuration was accepted and applyed, false otherwise.
     */
    virtual bool setConfiguration( const QString &data );

    /**
     * This method returns the current configuration of the resource in xml format.
     * If the resource has no configuration, an empty string is returned.
     */
    virtual QString configuration() const;

  public Q_SLOTS:
    /**
     * This method is called to quit the resource.
     *
     * Before the application is terminated @see aboutToQuit() is called,
     * which can be reimplemented to do some session cleanup (e.g. disconnecting
     * from groupware server).
     */
    void quit();

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
     * @param message An i18n'ed description of the status.
     */
    void changeStatus( Status status, const QString &message = QString() );

    /**
     * This method is called whenever the application is about to
     * quit.
     *
     * Reimplement this method to do session cleanup (e.g. disconnecting
     * from groupware server).
     */
    virtual void aboutToQuit();

  private:
    static QString parseArguments( int, char** );

    class Private;
    Private* const d;
};

}

#endif
