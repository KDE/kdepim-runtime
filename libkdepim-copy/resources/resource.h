/*
    This file is part of libkdepim.
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>

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

#ifndef KPIM_RESOURCE_H
#define KPIM_RESOURCE_H

#include "plugin.h"

namespace KPIM {

/**
 * @internal
 */

class Resource : public Plugin
{
public:
  /**
   * Constructor
   */
  Resource();

  /**
   * Destructor.
   */
  virtual ~Resource();

  /**
   * Returns a unique identifier.
   */
  virtual QString identifier() const;

  /**
   * Mark the resource to read-only.
   */
  virtual void setReadOnly( bool value );

  /**
   * Returns, if the resource is read-only.
   */
  virtual bool readOnly() const;

  /**
   * Mark the resource as fast. 
   */
  virtual void setFastResource( bool value );

  /**
   * Returns, if the resource is fast.
   */
  virtual bool fastResource() const;

  /**
   * Set the name of resource.
   */
  virtual void setName( const QString &name );

  /**
   * Returns the name of resource.
   */
  virtual QString name() const;

  /**
   * This method can be used by all resources to encrypt
   * their passwords for storing in a config file.
   */
  static QString encryptStr( const QString & );
  /**
   * This method can be used by all resources to decrypt
   * their passwords read from a config file.
   */
  static QString decryptStr( const QString & );

private:
  bool mReadOnly;
  bool mFastResource;
  QString mName;
};

}
#endif
