/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#ifndef __AKONOTES_RESOURCE_H__
#define __AKONOTES_RESOURCE_H__

#include "maildirresource.h"
//#include <agentfactory.h>

class AkonotesResource : public MaildirResource
{
  Q_OBJECT
  public:
    explicit AkonotesResource( const QString &id );
    ~AkonotesResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected:
    virtual QString itemMimeType() const;
};
#if 0
#if 1
class AkonotesResourceFactory : public Akonadi::AgentFactory<AkonotesResource>
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.kde.akonadi.AkonotesResource");
   //Q_INTERFACES( AkonotesResourceFactory )
  public: \
    explicit AkonotesResourceFactory( QObject * parent = 0 ) : Akonadi::AgentFactory< AkonotesResource >( "akonadi_akonotes_resource", parent ) {
      setObjectName(QLatin1String("akonadi_akonotes_resource") );
    }

};
#else
#if 0
#define AKONADI_AGENT_FACTORY( agentClass, catalogName ) \
class agentClass ## Factory : public Akonadi::AgentFactory< agentClass > \
{ \
  Q_OBJECT \
  Q_PLUGIN_METADATA(IID "org.kde.akonadi." # agentClass ) \
  Q_INTERFACES( agentClass ## Factory ) \
  public: \
    explicit agentClass ## Factory( QObject * parent = 0 ) : Akonadi::AgentFactory< agentClass >( # catalogName, parent ) {\
      setObjectName(QLatin1String(# catalogName) );\
    } \
}; \
#endif

AKONADI_AGENT_FACTORY( AkonotesResource, akonadi_akonotes_resource )
#endif
#endif
#endif
#endif
