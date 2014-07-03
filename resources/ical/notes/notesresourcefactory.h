/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef NOTESRESOURCEFACTORY_H
#define NOTESRESOURCEFACTORY_H

#include <agentfactory.h> 
#include <notesresource.h>
 
class NotesResourceFactory : public Akonadi::AgentFactory<NotesResource>
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.kde.akonadi.NotesResource");
public: 
    explicit NotesResourceFactory( QObject * parent = 0 );

};
#endif
