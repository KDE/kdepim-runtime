/*  -*- c++ -*-
    configmanager.h

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/


#ifndef _KMAIL_CONFIGMANAGER_H_
#define _KMAIL_CONFIGMANAGER_H_

#include <QObject>

#include <kdepimmacros.h>

namespace KPIM {

/**
 * @short Class for managing a set of config options.
 * @author Marc Mutz <mutz@kde.org>
 **/
class KDE_EXPORT ConfigManager : public QObject {
  Q_OBJECT
public:
  /** Commit changes to disk and emit changed() if necessary. */
  virtual void commit() = 0;
  /** Re-read the config from disk and forget changes. */
  virtual void rollback() = 0;

  /** Check whether there are any unsaved changes. */
  virtual bool hasPendingChanges() const = 0;

signals:
  /** Emitted whenever a commit changes any configure option */
  void changed();

protected:
  ConfigManager( QObject * parent=0, const char * name=0 );
  virtual ~ConfigManager();
};

}

#endif // _KMAIL_CONFIGMANAGER_H_
