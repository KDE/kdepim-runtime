/**
 * prefsmodule.h
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
#ifndef KOMPOSER_PREFSMODULE_H
#define KOMPOSER_PREFSMODULE_H

#include <kprefsdialog.h>
#include <kservice.h>
#include <qmap.h>
class QGroupBox;
class QListViewItem;

class KAboutData;
class KComboBox;

namespace Komposer {

  class PrefsModule : public KPrefsModule
  {
    Q_OBJECT
  public:
    PrefsModule( QWidget* parent=0, const char* name=0 );
    virtual const KAboutData* aboutData() const;
  };

  class EditorSelection : public KPrefsWid
  {
  Q_OBJECT

  public:
    EditorSelection( const QString &text, QString &reference, QWidget *parent );
    ~EditorSelection();

    void readConfig();
    void writeConfig();

    QGroupBox *groupBox() const;

  private slots:
    void slotActivated( const QString& );

  private:
    void setItem( const QString& );
  private:
    QString& m_reference;

    QGroupBox *m_box;
    KComboBox *m_editorsCombo;
    QMap<QString, KService::Ptr> m_services;
  };
}

#endif
