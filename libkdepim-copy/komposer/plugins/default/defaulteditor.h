/*
 * defaulteditor.h
 *
 * Copyright (C)  2004  Zack Rusin <zack@kde.org>
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

#ifndef DEFAULTEDITOR_H
#define DEFAULTEDITOR_H

#include "editor.h"

class QTextEdit;
class KFontAction;
class KFontSizeAction;
class KToggleAction;
class KActionCollection;


class DefaultEditor : public Komposer::Editor
{
  Q_OBJECT
public:
  DefaultEditor( QObject *parent, const char *name, const QStringList &args );
  ~DefaultEditor();

  virtual QWidget *widget();
  virtual QString  text() const;
public Q_SLOTS:
  virtual void setText( const QString &txt );
  virtual void changeSignature( const QString &txt );

  /**
   * Displays a file dialog and loads the selected file.
   */
  bool open();

  /**
   * Displays a file dialog and saves to the selected file.
   */
  bool saveAs();

  /**
   * Prints the current document
   */
  bool print();

  /**
   * Displays a color dialog and sets the text color to the selected value.
   */
  void formatColor();

  void checkSpelling();

  /**
   * @internal
   */
  void setAlignLeft( bool yes );

  /**
   * @internal
   */
  void setAlignRight( bool yes );

  /**
   * @internal
   */
  void setAlignCenter( bool yes );

  /**
   * @internal
   */
  void setAlignJustify( bool yes );

protected Q_SLOTS:
  /**
   * Creates the part's actions in the part's action collection.
   */
  void createActions( KActionCollection *ac );

  void updateActions();

  void updateFont();
  void updateCharFmt();
  void updateAligment();

private:
  QTextEdit *m_textEdit;

  KToggleAction *m_actionBold;
  KToggleAction *m_actionItalic;
  KToggleAction *m_actionUnderline;

  KFontAction *m_actionFont;
  KFontSizeAction *m_actionFontSize;

  KToggleAction *m_actionAlignLeft;
  KToggleAction *m_actionAlignRight;
  KToggleAction *m_actionAlignCenter;
  KToggleAction *m_actionAlignJustify;
};

#endif
