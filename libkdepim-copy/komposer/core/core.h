// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
/**
 * core.h
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
#ifndef KOMPOSER_CORE_H
#define KOMPOSER_CORE_H

#include <kparts/mainwindow.h>
#include <kparts/part.h>

#include <qptrlist.h>

namespace KParts {
  class PartManager;
}
class QWidgetStack;

namespace Komposer
{

  class Editor;

  /**
   * This class provides the interface to the Komposer core for the editor.
   */
  class Core : public KParts::MainWindow
  {
    Q_OBJECT
  public:
    Core( QWidget *parentWidget = 0, const char *name = 0 );
    virtual ~Core();

    /**
      Selects the given editor @param editor and raises the associated
      part.
     */
    virtual void selectEditor( Komposer::Editor* editor );
    /**
      This is an overloaded member function. It behaves essentially like the
      above function.
     */
    virtual void selectEditor( const QString& editor );

    /**
      Returns the pointer list of available editors.
     */
    virtual QPtrList<Komposer::Editor> editorList() const { return m_editors; }

    KParts::ReadWritePart* createPart( const char *libname );

  protected slots:
    void slotActivePartChanged( KParts::Part *part );
    void slotPreferences();
    void slotQuit();

  protected:
    //virtual void initWidgets();
    virtual void initWidgets();
    void loadSettings();
    void saveSettings();

    void loadEditors();
    void unloadEditors();
    void addEditor( Komposer::Editor *editor );
    void addPart( KParts::Part *part );

  private:
    QMap<QCString, KParts::ReadWritePart*> m_parts;
    KParts::PartManager* m_partManager;
    QWidgetStack* m_stack;
    Editor* m_currentEditor;
    QPtrList<Komposer::Editor> m_editors;
    KSettings::Dialog* m_dlg;

    class Private;
    Private *d;
};

}

#endif
