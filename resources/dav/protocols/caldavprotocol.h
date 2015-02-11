/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef CALDAVPROTOCOL_H
#define CALDAVPROTOCOL_H

#include "davmultigetprotocol.h"

class CaldavProtocol : public DavMultigetProtocol
{
public:
    CaldavProtocol();
    bool supportsPrincipals() const Q_DECL_OVERRIDE;
    bool useReport() const Q_DECL_OVERRIDE;
    bool useMultiget() const Q_DECL_OVERRIDE;
    QString principalHomeSet() const Q_DECL_OVERRIDE;
    QString principalHomeSetNS() const Q_DECL_OVERRIDE;
    QDomDocument collectionsQuery() const Q_DECL_OVERRIDE;
    QString collectionsXQuery() const Q_DECL_OVERRIDE;
    QList<QDomDocument> itemsQueries() const Q_DECL_OVERRIDE;
    QString mimeTypeForQuery(int index) const Q_DECL_OVERRIDE;
    QDomDocument itemsReportQuery(const QStringList &urls) const Q_DECL_OVERRIDE;
    QString responseNamespace() const Q_DECL_OVERRIDE;
    QString dataTagName() const Q_DECL_OVERRIDE;

    DavCollection::ContentTypes collectionContentTypes(const QDomElement &propstat) const Q_DECL_OVERRIDE;
    QString contactsMimeType() const Q_DECL_OVERRIDE;

private:
    QList<QDomDocument> mItemsQueries;
    QStringList mItemsMimeTypes;
};

#endif
