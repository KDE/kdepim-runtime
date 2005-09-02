/*
    This file is part of libkdepim.

    Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA 02110-1301, USA.
*/
#ifndef KPIM_CATEGORYHIERARCHYREADER_H
#define KPIM_CATEGORYHIERARCHYREADER_H

class QComboBox;
class QStringList;
class QString;
class Q3ListView;
class Q3ListViewItem;

namespace KPIM {

class CategoryHierarchyReader
{
  public:
    void read( QStringList categories );
    virtual ~CategoryHierarchyReader() { }
    static QStringList path( QString string );
  protected:
    CategoryHierarchyReader() { }
    virtual void clear() = 0;
    virtual void goUp() = 0;
    virtual void addChild( const QString &label ) = 0;
    virtual int depth() const = 0;
};

class CategoryHierarchyReaderQListView : public CategoryHierarchyReader
{
  public:
    CategoryHierarchyReaderQListView( Q3ListView *view, bool expandable = true,
                                      bool checkList = false )
        : mView( view ), mItem( 0 ), mExpandable( expandable ), 
          mCheckList( checkList ) { }
    virtual ~CategoryHierarchyReaderQListView() { }

  protected:
    virtual void clear();
    virtual void goUp();
    virtual void addChild( const QString &label );
    virtual int depth() const;
  private:
    Q3ListView *mView;
    Q3ListViewItem *mItem;
    const bool mExpandable;
    const bool mCheckList;
};

class CategoryHierarchyReaderQComboBox : public CategoryHierarchyReader
{
  public:
    CategoryHierarchyReaderQComboBox( QComboBox *box )
        : mBox( box ), mCurrentDepth( 0 ) { }
    virtual ~CategoryHierarchyReaderQComboBox() { }

  protected:
    virtual void clear();
    virtual void goUp();
    virtual void addChild( const QString &label );
    virtual int depth() const;
  private:
    QComboBox *mBox;
    int mCurrentDepth;
};

}

#endif
