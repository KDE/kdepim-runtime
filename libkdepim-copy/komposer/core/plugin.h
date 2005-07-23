// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
/**
 * plugin.h
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#ifndef KOMPOSER_PLUGIN_H
#define KOMPOSER_PLUGIN_H

#include <qobject.h>
#include <kxmlguiclient.h>

namespace Komposer
{
  class Core;

  class Plugin : public QObject,
                 virtual public KXMLGUIClient
  {
    Q_OBJECT
  public:
    virtual ~Plugin();

  signals:
    void statusMessage( const QString & );
    void readyForUnload();

  protected slots:
    /**
     * Called when a new message is created.
     */
    virtual void startedComposing();

    /**
     * Called after the send button has been pressed
     * and before the message has been sent.
     */
    virtual void sendClicked();

    /**
     * Called after the quit button has been pressed
     */
    virtual void quitClicked();

    virtual void aboutToUnload();

  protected:
    Core *core() const;
  protected:
    friend class PluginManager;
    Plugin( QObject *parent, const char *name, const QStringList& args = QStringList() );

  private:
    class Private;
    Private *d;
  };

}

#endif
