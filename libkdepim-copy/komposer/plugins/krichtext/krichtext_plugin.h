/**
 * krichtext_plugin.h
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 */

#ifndef KRICHTEXT_PLUGIN_H
#define KRICHTEXT_PLUGIN_H

#include "editor.h"

namespace KParts {
  class Part;
  class ReadWritePart;
}

class KRichTextPlugin : public Komposer::Editor
{
  Q_OBJECT
public:
  KRichTextPlugin( Komposer::Core* core, const char* name, const QStringList& );
  ~KRichTextPlugin();

  virtual KParts::Part* part();
  virtual QString       text() const;
public slots:
  virtual void setText( const QString& txt );
  virtual void changeSignature( const QString& txt );
private:
  KParts::ReadWritePart* m_part;
};

#endif
