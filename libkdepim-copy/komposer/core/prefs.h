/**
 * prefs.h
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
#ifndef KOMPOSER_PREFS_H
#define KOMPOSER_PREFS_H

#include <kprefs.h>

namespace Komposer
{

  class Prefs : public KPrefs
  {
  public:
    ~Prefs();

    /**
      Get instance of Prefs. It is made sure that there is only one
      instance (singleton design pattern).
    */
    static Prefs *self();
  private:
    /**
      Constructor disabled for public. Use self() to create a Prefs
      object.
    */
    Prefs();

    static Prefs *s_instance;

  public:
    // preferences data
    QString m_activeEditor;
    QStringList m_activeEditors;
  };

}

#endif
