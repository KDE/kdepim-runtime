/*
    This file is part of KOrganizer.
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

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

#ifndef _KPREFS_H
#define _KPREFS_H

#include <qcolor.h>
#include <qdatetime.h>
#include <qfont.h>
#include <qpoint.h>
#include <qptrlist.h>
#include <qrect.h>
#include <qsize.h>
#include <qstringlist.h>
#include <qvariant.h>
#include <kconfig.h>
#include <kglobalsettings.h>

class KConfig;

/**
  @short Class for storing a preferences setting
  @author Cornelius Schumacher
  @see KPref
  
  This class represents one preferences setting as used by @ref KPrefs.
  Subclasses of KPrefsItem implement storage functions for a certain type of
  setting. Normally you don't have to use this class directly. Use the special
  addItem() functions of KPrefs instead. If you subclass this class you will
  have to register instances with the function KPrefs::addItem().
*/
class KPrefsItem {
  public:
    typedef QValueList<KPrefsItem *> List;
  
    /**
      Constructor.
      
      @param group Config file group.
      @param name Config file key.
    */
    KPrefsItem( const QString &group, const QString &name )
      : mGroup( group ), mName( name ) {}

    /**
      Destructor.
    */
    virtual ~KPrefsItem() {}
    
    /**
      Return config file group.
    */
    QString group() const { return mGroup; }

    /**
      Return config file key.
    */
    QString name() const { return mName; }
    
    /**
      This function is called by @ref KPrefs to set this setting to its default
      value.
    */
    virtual void setDefault() = 0;

    /**
      This function is called by @ref KPrefs to read the value for this setting
      from a config file.
      value.
    */
    virtual void readConfig( KConfig* ) = 0;

    /**
      This function is called by @ref KPrefs to write the value of this setting
      to a config file.
    */
    virtual void writeConfig( KConfig* ) = 0;

    /**
      Read global default value.
    */
    virtual void readDefault( KConfig * ) = 0;

  protected:
    QString mGroup;
    QString mName;
};


template <typename T>
class KGenericPrefsItem : public KPrefsItem
{
  public:
    KGenericPrefsItem( const QString &group, const QString &name, T &reference,
                       T defaultValue)
      : KPrefsItem( group, name ), mReference( reference ),
        mDefault( defaultValue ), mLoadedValue( defaultValue ) {}

    virtual void setDefault()
    {
      mReference = mDefault;
    }

    virtual void writeConfig( KConfig *config )
    {
      if ( mReference != mLoadedValue ) {
        config->setGroup( mGroup );
        config->writeEntry( mName, mReference );
      }
    }

    void readDefault( KConfig *config )
    {
      config->setReadDefaults( true );
      readConfig( config );
      config->setReadDefaults( false );
      mDefault = mReference;
    }

  protected:
    T &mReference;
    T mDefault;
    T mLoadedValue;
};


/**
  @short Class for handling preferences settings for an application.
  @author Cornelius Schumacher
  @see KPrefsItem

  This class provides an interface to preferences settings. Preferences items
  can be registered by the addItem() function corresponding to the data type of
  the seetting. KPrefs then handles reading and writing of config files and
  setting of default values.

  Normally you will subclass KPrefs, add data members for the preferences
  settings and register the members in the constructor of the subclass.
  
  Example:
  <pre>
  class MyPrefs : public KPrefs {
    public:
      MyPrefs()
      {
        setCurrentGroup("MyGroup");
        addItemBool("MySetting1",mMyBool,false);
        addItemColor("MySetting2",mMyColor,QColor(1,2,3));
        
        setCurrentGroup("MyOtherGroup");
        addItemFont("MySetting3",mMyFont,QFont("helvetica",12));
      }
      
      bool mMyBool;
      QColor mMyColor;
      QFont mMyFont;
  }
  </pre>
  
  It might be convenient in many cases to make this subclass of KPrefs a
  singleton for global access from all over the application without passing
  references to the KPrefs object around.

  You can set all values to default values by calling @ref setDefaults(), write
  the data to the configuration file by calling @ref writeConfig() and read the
  data from the configuration file by calling @ref readConfig().

  If you have items, which are not covered by the existing addItem() functions
  you can add customized code for reading, writing and default setting by
  implementing the functions @ref usrSetDefaults(), @ref usrReadConfig() and
  @ref usrWriteConfig().
  
  Internally preferences settings are stored in instances of subclasses of
  @ref KPrefsItem. You can also add KPrefsItem subclasses for your own types
  and call the generic @ref addItem() to register them.
*/
  
class KPrefs {
  public:
    /**
      Constructor.
      
      @param configname name of config file. If no name is given, the default
                        config file as returned by kapp()->config() is used.
    */
    KPrefs( const QString &configname = QString::null );

    /**
      Destructor
    */
    virtual ~KPrefs();
  
    /**
      Set preferences to default values. All registered items are set to their
      default values.
    */
    void setDefaults();
  
    /**
      Read preferences from config file. All registered items are set to the
      values read from disk.
    */
    void readConfig();

    /**
      Write preferences to config file. The values of all registered items are
      written to disk.
    */
    void writeConfig();

    /**
      Set the config file group for subsequent addItem() calls. It is valid
      until setCurrentGroup() is called with a new argument. Call this before
      you add any items. The default value is "No Group".
    */
    void setCurrentGroup( const QString &group );

    /**
      Register a custom @ref KPrefsItem.
    */
    void addItem( KPrefsItem* );

    /**
      Register an item of type QString.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemString( const QString &key, QString &reference,
                        const QString &defaultValue = "" );

    /**
      Register a password item of type QString. The string value is written 
      encrypted to the config file. Note that the current encryption scheme 
      is very weak.

      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemPassword( const QString &key, QString &reference,
                          const QString &defaultValue = "" );

    /**
      Register a path item of type QString. The string value is interpreted
      as a path. This means, dollar expension is activated for this value, so
      that e.g. $HOME gets expanded. 

      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemPath( const QString &key, QString &reference,
                      const QString &defaultValue = "" );

    /**
      Register a property item of type QVariant. Note that only the following
      QVariant types are allowed: String, StringList, Font, Point, Rect, Size,
      Color, Int, UInt, Bool, Double, DateTime and Date.

      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemProperty( const QString &key, QVariant &reference,
                          const QVariant &defaultValue = QVariant() );

    /**
      Register an item of type bool.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemBool( const QString &key, bool &reference,
                      bool defaultValue = false );

    /**
      Register an item of type int.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemInt( const QString &key, int &reference,
                     int defaultValue = 0 );

    /**
      Register an item of type unsigned int.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemUInt( const QString &key, unsigned int &reference,
                      unsigned int defaultValue = 0 );

    /**
      Register an item of type long.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemLong( const QString &key, long &reference,
                      long defaultValue = 0 );

    /**
      Register an item of type unsigned long.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemULong( const QString &key, unsigned long &reference,
                       unsigned long defaultValue = 0 );

    /**
      Register an item of type double.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemDouble( const QString &key, double &reference,
                        double defaultValue = 0.0 );

    /**
      Register an item of type QColor.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemColor( const QString &key, QColor &reference,
                       const QColor &defaultValue = QColor( 128, 128, 128 ) );

    /**
      Register an item of type QFont.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemFont( const QString &key, QFont &reference,
                      const QFont &defaultValue = KGlobalSettings::generalFont() );

    /**
      Register an item of type QRect.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemRect( const QString &key, QRect &reference,
                      const QRect &defaultValue = QRect() );

    /**
      Register an item of type QPoint.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemPoint( const QString &key, QPoint &reference,
                       const QPoint &defaultValue = QPoint() );

    /**
      Register an item of type QSize.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemSize( const QString &key, QSize &reference,
                      const QSize &defaultValue = QSize() );

    /**
      Register an item of type QDateTime.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemDateTime( const QString &key, QDateTime &reference,
                          const QDateTime &defaultValue = QDateTime() );

    /**
      Register an item of type QStringList.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemStringList( const QString &key, QStringList &reference,
                            const QStringList &defaultValue = QStringList() );

    /**
      Register an item of type QValueList<int>.
      
      @param key Key used in config file.
      @param reference Pointer to the variable, which is set by readConfig()
                       and setDefaults() calls and read by writeConfig() calls.
      @param defaultValue Default value, which is used by setDefaults() and
                          when the config file does not yet contain the key of
                          this item.
    */
    void addItemIntList( const QString &key, QValueList<int> &reference,
                         const QValueList<int> &defaultValue = QValueList<int>() );

    /**
      Return the @ref KConfig object used for reading and writing the settings.
    */
    KConfig *config() const;

    /**
      Return list of items managed by this KPrefs object.
    */
    KPrefsItem::List items() const { return mItems; }

  protected:
    /**
      Implemented by subclasses that use special defaults.
    */
    virtual void usrSetDefaults() {};

    /**
      Implemented by subclasses that read special config values.
    */
    virtual void usrReadConfig() {};

    /**
      Implemented by subclasses that write special config values.
    */
    virtual void usrWriteConfig() {};

  private:
    QString mCurrentGroup;

    KConfig *mConfig;  // pointer to KConfig object

    KPrefsItem::List mItems;

    class Private;
    Private *d;
};

#endif
