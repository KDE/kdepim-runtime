/*
    This file is part of libkdepim.

    Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef DIFFALGO_H
#define DIFFALGO_H

#include <qvaluelist.h>
#include <kdepimmacros.h>

namespace KPIM {

/**
  DiffAlgo and DiffAlgoDisplay work together for displaying differences between
  two PIM objects like contacts, events or todos.
  DiffAlgo is the bas class for the diffing algorithm and DiffAlgoDisplay is
  responsible for representation. The separation makes it possible to use one
  display for all diffing algorithm and vice versa.
 */
class DiffAlgoDisplay
{
  public:

    /**
      Is called on the start of the diff.
     */
    virtual void begin() = 0;

    /**
      Is called on the end of the diff.
     */
    virtual void end() = 0;

    /**
      Sets the title of the left data source.
     */
    virtual void setLeftSourceTitle( const QString &title ) = 0;

    /**
      Sets the title of the right data source.
     */
    virtual void setRightSourceTitle( const QString &title ) = 0;

    /**
      Adds a field which is only available in the left data source.
     */
    virtual void additionalLeftField( const QString &id, const QString &value ) = 0;

    /**
      Adds a field which is only available in the right data source.
     */
    virtual void additionalRightField( const QString &id, const QString &value ) = 0;

    /**
      Adds a conflict between two fields.
     */
    virtual void conflictField( const QString &id, const QString &leftValue,
                                const QString &rightValue ) = 0;
};


class KDE_EXPORT DiffAlgo
{
  public:
    /**
      Destructor.
     */
    virtual ~DiffAlgo() {};

    /**
      Starts the diffing algorithm.
     */
    virtual void run() = 0;

    /**
      Must be called on the start of the diff.
     */
    void begin();

    /**
      Must be called on the end of the diff.
     */
    void end();

    /**
      Sets the title of the left data source.
     */
    void setLeftSourceTitle( const QString &title );

    /**
      Sets the title of the right data source.
     */
    void setRightSourceTitle( const QString &title );

    /**
      Adds a field which is only available in the left data source.
     */
    void additionalLeftField( const QString &id, const QString &value );

    /**
      Adds a field which is only available in the right data source.
     */
    void additionalRightField( const QString &id, const QString &value );

    /**
      Adds a conflict between two fields.
     */
    void conflictField( const QString &id, const QString &leftValue,
                        const QString &rightValue );

    void addDisplay( DiffAlgoDisplay *display );
    void removeDisplay( DiffAlgoDisplay *display );


  private:
    QValueList<DiffAlgoDisplay*> mDisplays;
};

}

#endif
