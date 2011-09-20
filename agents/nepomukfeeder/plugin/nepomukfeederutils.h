/*
    Copyright (C) 2011  Christian Mollekopf <chrigi_1@fastmail.fm>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NEPOMUKFEEDERUTILS_H
#define NEPOMUKFEEDERUTILS_H

#include <dms-copy/simpleresource.h>

namespace Nepomuk
{
  class SimpleResourceGraph;
}
class QString;
class QStringList;

/**
 * A namespace for various helper functions
 */
namespace NepomukFeederUtils
{
    /** Adds tags to @p resource based on the given string list*/
    void tagsFromCategories(const QStringList& categories, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph);
    void setIcon(const QString& iconName, Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph);
    Nepomuk::SimpleResource addTag(Nepomuk::SimpleResource& res, Nepomuk::SimpleResourceGraph& graph, const QString& identifier, const QString &label = QString() );
    /** Creates a PersonContact object for the given name and address,
    *   adds it to @param graph and returns it's url.
    */
    Nepomuk::SimpleResource addContact( const QString &email, const QString &name, Nepomuk::SimpleResourceGraph &graph );
}

#endif // NEPOMUKFEEDERUTILS_H
