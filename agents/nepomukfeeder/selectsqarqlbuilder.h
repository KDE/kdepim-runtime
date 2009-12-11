/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef SELECTSPARQLBUILDER_H
#define SELECTSPARQLBUILDER_H

#include "sparqlbuilder.h"

#include <QStringList>

/** SPARQL query builder for SELECT queries. */
class SelectSparqlBuilder : public SparqlBuilder
{
  public:
    SelectSparqlBuilder() : m_distinct( false ) {}
    void setDistinct( bool distinct ) { m_distinct = distinct; }
    void addQueryVariable( const QString &variable ) { m_variables.append( variable ); }

    /** Returns the SPARQL query. */
    QString query() const;

  private:
    QStringList m_variables;
    bool m_distinct;
};

#endif
