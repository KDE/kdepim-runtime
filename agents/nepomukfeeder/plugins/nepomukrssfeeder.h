/*
    Copyright (c) 2013 Dan Vrátil <dvratil@redhat.com>

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

#ifndef NEPOMUKRSSFEEDER_H
#define NEPOMUKRSSFEEDER_H

#include <nepomukfeederplugin.h>

namespace Akonadi {

class NepomukRSSFeeder : public NepomukFeederPlugin
{
    Q_OBJECT
    Q_INTERFACES( Akonadi::NepomukFeederPlugin )
public:

    explicit NepomukRSSFeeder(QObject* parent, const QVariantList &);
    virtual ~NepomukRSSFeeder();

    virtual void updateItem(const Item& item, Nepomuk2::SimpleResource& res, Nepomuk2::SimpleResourceGraph& graph);

private:
    void addTag( const QString &tagName, const QString &tagLabel, const QString &icon, Nepomuk2::SimpleResource &res, Nepomuk2::SimpleResourceGraph& graph );
};

} /* namespace Akonadi */

#endif // NEPOMUKRSSFEEDER_H
