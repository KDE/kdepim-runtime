/*
    This file is part of KOrganizer.
    Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>

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

// $Id$

#include <qcolor.h>

#include <kconfig.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <kdebug.h>

#include "kprefs.h"

class KPrefsItemBool : public KGenericPrefsItem<bool>
{
  public:
    KPrefsItemBool( const QString &group, const QString &name, bool &reference,
                    bool defaultValue = true )
      : KGenericPrefsItem<bool>( group, name, reference, defaultValue ) {}
    
    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readBoolEntry( mName, mDefault );
    }
};


class KPrefsItemInt : public KGenericPrefsItem<int>
{
  public:
    KPrefsItemInt( const QString &group, const QString &name, int &reference,
                   int defaultValue = 0 )
      : KGenericPrefsItem<int>( group, name, reference, defaultValue ) {}
    
    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readNumEntry( mName, mDefault );
    }
};


class KPrefsItemColor : public KGenericPrefsItem<QColor>
{
  public:
    KPrefsItemColor( const QString &group, const QString &name,
                     QColor &reference,
                     const QColor &defaultValue = QColor( 128, 128, 128 ) )
      : KGenericPrefsItem<QColor>( group, name, reference, defaultValue ) {}
    
    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readColorEntry( mName, &mDefault );
    }
};


class KPrefsItemFont : public KGenericPrefsItem<QFont>
{
  public:
    KPrefsItemFont( const QString &group, const QString &name, QFont &reference,
                    const QFont &defaultValue = QFont( "helvetica", 12 ) )
      : KGenericPrefsItem<QFont>( group, name, reference, defaultValue ) {}
    
    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readFontEntry( mName, &mDefault );
    }
};


class KPrefsItemString : public KGenericPrefsItem<QString>
{
  public:
    KPrefsItemString( const QString &group, const QString &name,
                      QString &reference,
                      const QString &defaultValue = QString::null,
                      bool isPassword = false )
      : KGenericPrefsItem<QString>( group, name, reference, defaultValue ),
        mIsPassword( isPassword ) {}
    
    void writeConfig(KConfig *config)
    {
      config->setGroup( mGroup );
      if ( mIsPassword )
        config->writeEntry( mName, endecryptStr( mReference ) );
      else
        config->writeEntry( mName, mReference );
    }

    void readConfig(KConfig *config)
    {
      config->setGroup( mGroup );

      if ( mIsPassword ) {
        QString value = config->readEntry( mName, endecryptStr( mDefault ) );
        mReference = endecryptStr( value );
      } else {
        mReference = config->readEntry( mName, mDefault );
      }
    }

  protected:
    QString endecryptStr( const QString &aStr )
    {
      QString result;
      for ( uint i = 0; i < aStr.length(); i++ )
        result += (aStr[i].unicode() < 0x20) ? aStr[i] :
          QChar(0x1001F - aStr[i].unicode());
      return result;
    }

  private:
    bool mIsPassword;
};


class KPrefsItemStringList : public KGenericPrefsItem<QStringList>
{
  public:
    KPrefsItemStringList( const QString &group, const QString &name,
                          QStringList &reference,
                          const QStringList &defaultValue = QStringList() )
      : KGenericPrefsItem<QStringList>( group, name, reference, defaultValue ) {}
    
    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readListEntry( mName );
    }
};


class KPrefsItemIntList : public KGenericPrefsItem<QValueList<int> >
{
  public:
    KPrefsItemIntList( const QString &group, const QString &name,
                       QValueList<int> &reference,
                       const QValueList<int> &defaultValue = QValueList<int>() )
      : KGenericPrefsItem<QValueList<int> >( group, name, reference, defaultValue ) {}
    
    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readIntListEntry( mName );
    }
};



QString *KPrefs::mCurrentGroup = 0;

KPrefs::KPrefs(const QString &configname)
{
  if (!configname.isEmpty()) {
    mConfig = new KConfig(locateLocal("config",configname));
  } else {
    mConfig = KGlobal::config();
  }

  mItems.setAutoDelete(true);

  // Set default group
  if (mCurrentGroup == 0) mCurrentGroup = new QString("No Group");
}

KPrefs::~KPrefs()
{
  if (mConfig != KGlobal::config()) {
    delete mConfig;
  }
}

void KPrefs::setCurrentGroup(const QString &group)
{
  if (mCurrentGroup) delete mCurrentGroup;
  mCurrentGroup = new QString(group);
}

KConfig *KPrefs::config() const
{
  return mConfig;
}

void KPrefs::setDefaults()
{
  KPrefsItem *item;
  for(item = mItems.first();item;item = mItems.next()) {
    item->setDefault();
  }

  usrSetDefaults();
}

void KPrefs::readConfig()
{
  KPrefsItem *item;
  for(item = mItems.first();item;item = mItems.next()) {
    item->readConfig(mConfig);
  }

  usrReadConfig();
}

void KPrefs::writeConfig()
{
  KPrefsItem *item;
  for(item = mItems.first();item;item = mItems.next()) {
    item->writeConfig(mConfig);
  }

  usrWriteConfig();

  mConfig->sync();
}


void KPrefs::addItem(KPrefsItem *item)
{
  mItems.append(item);
}

void KPrefs::addItemBool(const QString &key,bool &reference,bool defaultValue)
{
  addItem(new KPrefsItemBool(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemInt(const QString &key,int &reference,int defaultValue)
{
  addItem(new KPrefsItemInt(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemColor(const QString &key,QColor &reference,const QColor &defaultValue)
{
  addItem(new KPrefsItemColor(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemFont(const QString &key,QFont &reference,const QFont &defaultValue)
{
  addItem(new KPrefsItemFont(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemString(const QString &key,QString &reference,const QString &defaultValue)
{
  addItem(new KPrefsItemString(*mCurrentGroup,key,reference,defaultValue,false));
}

void KPrefs::addItemPassword(const QString &key,QString &reference,const QString &defaultValue)
{
  addItem(new KPrefsItemString(*mCurrentGroup,key,reference,defaultValue,true));
}

void KPrefs::addItemStringList(const QString &key,QStringList &reference,
                               const QStringList &defaultValue)
{
  addItem(new KPrefsItemStringList(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemIntList(const QString &key,QValueList<int> &reference,
                            const QValueList<int> &defaultValue)
{
  addItem(new KPrefsItemIntList(*mCurrentGroup,key,reference,defaultValue));
}
