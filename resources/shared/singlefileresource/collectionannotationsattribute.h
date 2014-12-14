/*
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_COLLECTIONANNOTATIONSATTRIBUTE_H
#define AKONADI_COLLECTIONANNOTATIONSATTRIBUTE_H

#include <attribute.h>
#include "akonadi-singlefileresource_export.h"
#include <QtCore/QMap>

namespace Akonadi {

class AKONADI_SINGLEFILERESOURCE_EXPORT CollectionAnnotationsAttribute : public Akonadi::Attribute
{
  public:
    CollectionAnnotationsAttribute();
    CollectionAnnotationsAttribute( const QMap<QByteArray, QByteArray> &annotations );
    void setAnnotations( const QMap<QByteArray, QByteArray> &annotations );
    QMap<QByteArray, QByteArray> annotations() const;
    virtual QByteArray type() const Q_DECL_OVERRIDE;
    virtual Attribute *clone() const Q_DECL_OVERRIDE;
    virtual QByteArray serialized() const Q_DECL_OVERRIDE;
    virtual void deserialize( const QByteArray &data ) Q_DECL_OVERRIDE;

  private:
    QMap<QByteArray, QByteArray> mAnnotations;
};

}

#endif
