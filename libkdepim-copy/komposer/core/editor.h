/**
 * editor.h
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

#ifndef KOMPOSER_EDITOR_H
#define KOMPOSER_EDITOR_H

#include <qobject.h>
#include <kxmlguiclient.h>
#include <qstringlist.h>

namespace KParts {
  class Part;
}

namespace Komposer {

  class Core;

  class Editor : public QObject, virtual public KXMLGUIClient
  {
    Q_OBJECT
  public:
    virtual ~Editor();

    /**
     *  Sets the identifier.
     */
    void setIdentifier( const QString &identifier );

    /**
     * Returns the identifier. It is used as argument for several
     * methods of Komposer core.
     */
    QString identifier() const;


    /**
     * This is the magic function that all derivatives have to reimplement.
     * It returns the actual editor component.
     */
    virtual KParts::Part* part() =0;

    /**
     * Returns the full text inside the editor.
     */
    virtual QString text() const =0;

    /**
     * Return the weight of the editor. The higher the weight the lower it will
     * be displayed in the combobox. The default implementation returns 0.
     */
    virtual int weight() const { return 0; }

    /**
     * This function is called when the plugin is selected by the user before the
     * widget of the KPart belonging to the plugin is raised.
     */
    virtual void select();

    /**
     * Reimplement this method and return a @ref QStringList of all config
     * modules your application part should offer via Komposer. Note that the
     * part and the module will have to take care for config syncing themselves.
     * Usually @p DCOP used for that purpose.
     *
     * @note Make sure you offer the modules in the form:
     * <code>"pathrelativetosettings/mysettings.desktop"</code>
     */
    virtual QStringList configModules() const { return QStringList(); }

    Core* core() const;

  public slots:
    /**
     * Sets the text of the opened editor.
     * Most commonly used on replaying.
     * If any text is present if will be deleted.
     */
    virtual void setText( const QString& txt ) =0;

    /**
     * Changes currently used signature. If no signature is present
     * a new one should be appened.
     */
    virtual void changeSignature( const QString& txt ) =0;

  protected:
    Editor( Core* core, QObject* parent, const char* name );

  private:
    class Private;
    Private* d;
  };

}

#endif
