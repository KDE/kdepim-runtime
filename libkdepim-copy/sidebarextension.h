/* This file is part of the KDE project
   Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>

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
#ifndef  SIDEBAREXTENSION_H
#define  SIDEBAREXTENSION_H

#include <qobject.h>

class QString;
class QWidget;

namespace KParts
{

  class ReadOnlyPart;

  /**
   * Provides a way to export a widget which will be displayed in Kontacts
   * stackview at the left
   **/
  class SideBarExtension : public QObject
  {
    Q_OBJECT

    public:
      SideBarExtension(KParts::ReadOnlyPart *parent, const char* name);
      ~SideBarExtension();

      /**
       * Pass the sidebar widget here. It will be reparented by kontact
       **/
      void setWidget(QWidget *widget) { m_child = widget; };
      /**
       * Set the display name of the Service your part is to provide
       **/
      void setDisplayName(const QString& name) { m_name = name; };
      /**
       * Set the name of the icon. 
       **/
      void setIcon(const QString& icon) { m_icon = icon; };

      /**
       * Retrieve a pointer to the widget
       **/
      QWidget* widget() const { return m_child; };

      /**
       * Retrieve the display name
       **/
      QString displayName() const { return m_name; };

      /**
       * Retrieve the name of the icon
       **/
      QString icon() const { return m_icon; };

    private:
      QWidget* m_child;
      QString  m_name;
      QString  m_icon;

      class SideBarExtensionPrivate;
      SideBarExtensionPrivate *d;
  };
};
#endif // SIDEBAREXTENSION_H 

// vim: ts=2 sw=2 et
