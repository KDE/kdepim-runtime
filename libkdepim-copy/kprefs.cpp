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

#include <qcolor.h>

#include <kconfig.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kdebug.h>

#include "kprefs.h"

class KPrefsItemString : public KGenericPrefsItem<QString>
{
  public:

    enum Type { Normal, Password, Path };

    KPrefsItemString( const QString &group, const QString &name,
                      QString &reference,
                      const QString &defaultValue = QString::null,
                      Type type = Normal )
      : KGenericPrefsItem<QString>( group, name, reference, defaultValue ),
        mType( type ) {}

    void writeConfig( KConfig *config )
    {
      if ( mLoadedValue != mReference ) {
        config->setGroup( mGroup );
        if ( mType == Path )
          config->writePathEntry( mName, mReference );
        else if ( mType == Password )
          config->writeEntry( mName, endecryptStr( mReference ) );
        else
          config->writeEntry( mName, mReference );
      }
    }

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );

      if ( mType == Path )
        mReference = config->readPathEntry( mName, mDefault );
      else if ( mType == Password ) {
        QString value = config->readEntry( mName, endecryptStr( mDefault ) );
        mReference = endecryptStr( value );
      } else
        mReference = config->readEntry( mName, mDefault );

      mLoadedValue = mReference;
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
    Type mType;
};


class KPrefsItemProperty : public KGenericPrefsItem<QVariant>
{
  public:
    KPrefsItemProperty( const QString &group, const QString &name,
                        QVariant &reference, QVariant defaultValue = 0 )
      : KGenericPrefsItem<QVariant>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readPropertyEntry( mName, mDefault );
      mLoadedValue = mReference;
    }
};


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
      mLoadedValue = mReference;
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
      mLoadedValue = mReference;
    }
};


class KPrefsItemUInt : public KGenericPrefsItem<unsigned int>
{
  public:
    KPrefsItemUInt( const QString &group, const QString &name,
                    unsigned int &reference, unsigned int defaultValue = 0 )
      : KGenericPrefsItem<unsigned int>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readUnsignedNumEntry( mName, mDefault );
      mLoadedValue = mReference;
    }
};


class KPrefsItemLong : public KGenericPrefsItem<long>
{
  public:
    KPrefsItemLong( const QString &group, const QString &name, long &reference,
                    long defaultValue = 0 )
      : KGenericPrefsItem<long>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readLongNumEntry( mName, mDefault );
      mLoadedValue = mReference;
    }
};


class KPrefsItemULong : public KGenericPrefsItem<unsigned long>
{
  public:
    KPrefsItemULong( const QString &group, const QString &name,
                     unsigned long &reference, unsigned long defaultValue = 0 )
      : KGenericPrefsItem<unsigned long>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readUnsignedLongNumEntry( mName, mDefault );
      mLoadedValue = mReference;
    }
};


class KPrefsItemDouble : public KGenericPrefsItem<double>
{
  public:
    KPrefsItemDouble( const QString &group, const QString &name,
                      double &reference, double defaultValue = 0 )
      : KGenericPrefsItem<double>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readDoubleNumEntry( mName, mDefault );
      mLoadedValue = mReference;
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
      mLoadedValue = mReference;
    }
};


class KPrefsItemFont : public KGenericPrefsItem<QFont>
{
  public:
    KPrefsItemFont( const QString &group, const QString &name, QFont &reference,
                    const QFont &defaultValue = KGlobalSettings::generalFont() )
      : KGenericPrefsItem<QFont>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readFontEntry( mName, &mDefault );
      mLoadedValue = mReference;
    }
};


class KPrefsItemRect : public KGenericPrefsItem<QRect>
{
  public:
    KPrefsItemRect( const QString &group, const QString &name, QRect &reference,
                    const QRect &defaultValue = QRect() )
      : KGenericPrefsItem<QRect>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readRectEntry( mName, &mDefault );
      mLoadedValue = mReference;
    }
};


class KPrefsItemPoint : public KGenericPrefsItem<QPoint>
{
  public:
    KPrefsItemPoint( const QString &group, const QString &name, QPoint &reference,
                     const QPoint &defaultValue = QPoint() )
      : KGenericPrefsItem<QPoint>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readPointEntry( mName, &mDefault );
      mLoadedValue = mReference;
    }
};


class KPrefsItemSize : public KGenericPrefsItem<QSize>
{
  public:
    KPrefsItemSize( const QString &group, const QString &name, QSize &reference,
                    const QSize &defaultValue = QSize() )
      : KGenericPrefsItem<QSize>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readSizeEntry( mName, &mDefault );
      mLoadedValue = mReference;
    }
};


class KPrefsItemDateTime : public KGenericPrefsItem<QDateTime>
{
  public:
    KPrefsItemDateTime( const QString &group, const QString &name,
                        QDateTime &reference,
                        const QDateTime &defaultValue = QDateTime() )
      : KGenericPrefsItem<QDateTime>( group, name, reference, defaultValue ) {}

    void readConfig( KConfig *config )
    {
      config->setGroup( mGroup );
      mReference = config->readDateTimeEntry( mName, &mDefault );
      mLoadedValue = mReference;
    }
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
      if ( !config->hasKey( mName ) )
        mReference = mDefault;
      else
        mReference = config->readListEntry( mName );
      mLoadedValue = mReference;
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
      if ( !config->hasKey( mName ) )
        mReference = mDefault;
      else
        mReference = config->readIntListEntry( mName );
      mLoadedValue = mReference;
    }
};



KPrefs::KPrefs( const QString &configname )
  : mCurrentGroup( "No Group" )
{
  if ( !configname.isEmpty() ) {
    mConfig = new KConfig( configname );
  } else {
    mConfig = KGlobal::config();
  }

  mItems.setAutoDelete( true );
}

KPrefs::~KPrefs()
{
  if ( mConfig != KGlobal::config() ) {
    delete mConfig;
  }
}

void KPrefs::setCurrentGroup( const QString &group )
{
  mCurrentGroup = group;
}

KConfig *KPrefs::config() const
{
  return mConfig;
}

void KPrefs::setDefaults()
{
  KPrefsItem *item;
  for( item = mItems.first(); item; item = mItems.next() ) {
    item->setDefault();
  }

  usrSetDefaults();
}

void KPrefs::readConfig()
{
  KPrefsItem *item;
  for( item = mItems.first(); item; item = mItems.next() ) {
    item->readConfig( mConfig );
  }

  usrReadConfig();
}

void KPrefs::writeConfig()
{
  KPrefsItem *item;
  for( item = mItems.first(); item; item = mItems.next() ) {
    item->writeConfig( mConfig );
  }

  usrWriteConfig();

  mConfig->sync();

  readConfig();
}


void KPrefs::addItem( KPrefsItem *item )
{
  mItems.append( item );
  item->readDefault( mConfig );
}

void KPrefs::addItemString( const QString &key, QString &reference, const QString &defaultValue )
{
  addItem( new KPrefsItemString( mCurrentGroup, key, reference, defaultValue, KPrefsItemString::Normal ) );
}

void KPrefs::addItemPassword( const QString &key, QString &reference, const QString &defaultValue )
{
  addItem( new KPrefsItemString( mCurrentGroup, key, reference, defaultValue, KPrefsItemString::Password ) );
}

void KPrefs::addItemPath( const QString &key, QString &reference, const QString &defaultValue )
{
  addItem( new KPrefsItemString( mCurrentGroup, key, reference, defaultValue, KPrefsItemString::Path ) );
}

void KPrefs::addItemProperty( const QString &key, QVariant &reference,
                              const QVariant &defaultValue )
{
  addItem( new KPrefsItemProperty( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemBool( const QString &key, bool &reference, bool defaultValue )
{
  addItem( new KPrefsItemBool( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemInt( const QString &key, int &reference, int defaultValue )
{
  addItem( new KPrefsItemInt( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemUInt( const QString &key, unsigned int &reference,
                          unsigned int defaultValue )
{
  addItem( new KPrefsItemUInt( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemLong( const QString &key, long &reference, long defaultValue )
{
  addItem( new KPrefsItemLong( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemULong( const QString &key, unsigned long &reference,
                           unsigned long defaultValue )
{
  addItem( new KPrefsItemULong( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemDouble( const QString &key, double &reference,
                            double defaultValue )
{
  addItem( new KPrefsItemDouble( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemColor( const QString &key, QColor &reference, const QColor &defaultValue )
{
  addItem( new KPrefsItemColor( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemFont( const QString &key, QFont &reference, const QFont &defaultValue )
{
  addItem( new KPrefsItemFont( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemRect( const QString &key, QRect &reference,
                          const QRect &defaultValue )
{
  addItem( new KPrefsItemRect( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemPoint( const QString &key, QPoint &reference,
                           const QPoint &defaultValue )
{
  addItem( new KPrefsItemPoint( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemSize( const QString &key, QSize &reference,
                          const QSize &defaultValue )
{
  addItem( new KPrefsItemSize( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemDateTime( const QString &key, QDateTime &reference,
                              const QDateTime &defaultValue )
{
  addItem( new KPrefsItemDateTime( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemStringList( const QString &key, QStringList &reference,
                                const QStringList &defaultValue )
{
  addItem( new KPrefsItemStringList( mCurrentGroup, key, reference, defaultValue ) );
}

void KPrefs::addItemIntList( const QString &key, QValueList<int> &reference,
                             const QValueList<int> &defaultValue )
{
  addItem( new KPrefsItemIntList( mCurrentGroup, key, reference, defaultValue ) );
}
