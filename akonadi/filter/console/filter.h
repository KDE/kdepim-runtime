/******************************************************************************
 *
 *  File : filter.h
 *  Created on Sat 13 Jun 2009 06:08:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _FILTER_H_
#define _FILTER_H_

#include <akonadi/collection.h>

#include <QVariantList>

namespace Akonadi
{

namespace Filter
{

  class Program;
  class ComponentFactory;

  namespace UI
  {
    class EditorFactory;
  } // namespace UI

} // namespace Filter
  
} // namespace Akonadi

class Filter
{
public:
  Filter();
  virtual ~Filter();
protected:
  QString mId;
  QList< Akonadi::Collection * > mCollections;
  QString mFileName;
  QString mMimeType;
  Akonadi::Filter::Program * mProgram;
  Akonadi::Filter::ComponentFactory * mComponentFactory;
  Akonadi::Filter::UI::EditorFactory * mEditorFactory;
public:
  const QString & id() const
  {
    return mId;
  }

  void setId( const QString &id )
  {
    mId = id;
  }

  const QString & mimeType() const
  {
    return mMimeType;
  }

  void setMimeType( const QString &mimeType )
  {
    mMimeType = mimeType;
  }

  QList< Akonadi::Collection * > * collections()
  {
    return &mCollections;
  }

  QVariantList collectionsAsVariantList();

  bool hasCollection( Akonadi::Collection::Id id );
  void addCollection( Akonadi::Collection * collection );
  void removeCollection( Akonadi::Collection::Id id );
  Akonadi::Collection * findCollection( Akonadi::Collection::Id id );
  void removeAllCollections();

  const QString & fileName() const
  {
    return mFileName;
  }

  void setFileName( const QString &fileName )
  {
    mFileName = fileName;
  }

  Akonadi::Filter::Program * program()
  {
    return mProgram;
  }

  Akonadi::Filter::ComponentFactory * componentFactory()
  {
    return mComponentFactory;
  }

  void setComponentFactory( Akonadi::Filter::ComponentFactory * componentFactory )
  {
    mComponentFactory = componentFactory;
  }

  Akonadi::Filter::UI::EditorFactory * editorFactory()
  {
    return mEditorFactory;
  }

  void setEditorFactory( Akonadi::Filter::UI::EditorFactory * editorFactory )
  {
    mEditorFactory = editorFactory;
  }

  void setProgram( Akonadi::Filter::Program * prog );

};

#endif //!_FILTER_H_
