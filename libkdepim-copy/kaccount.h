/*  -*- c++ -*-
    kaccount.h

    This file is part of KMail, the KDE mail client.
    Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KACCOUNT
#define __KACCOUNT

#include <qstring.h>

class KConfig;

class KAccount
{
  public:
    /** Type information */
    enum Type {
      Imap,
      MBox,
      Maildir,
      News,
      DImap,
      Other
    };

    KAccount( const uint id = 0, const QString &name = QString::null,
       const Type type = Other );
    
    /**
     * Get/Set name
     */ 
    QString name() const { return mName; }
    void setName( const QString& name ) { mName = name; }
    
    /**
     * Get/Set id
     */ 
    uint id() const { return mId; }
    void setId( const uint id ) { mId = id; }

    /**
     * Get/Set type
     */ 
    Type type() const { return mType; }
    void setType( const Type type ) { mType = type; }

    /**
     * Save the settings
     * If the group is empty it must be preset in the KConfig
     */
    void writeConfig( KConfig &config, const QString &group = QString::null ); 

    /**
     * Read the settings
     * If the group is empty it must be preset in the KConfig
     */
    void readConfig( KConfig &config, const QString &group = QString::null ); 

  protected:
    uint mId;
    QString mName;
    Type mType; 
};

#endif
