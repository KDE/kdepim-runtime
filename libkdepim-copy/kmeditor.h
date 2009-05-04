/**
 * kmeditor.h
 *
 * Copyright 2007 Laurent Montel <montel@kde.org>
 * Copyright 2008 Thomas McGuire <mcguire@kde.org>
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

#ifndef KDEPIM_KMEDITOR_H
#define KDEPIM_KMEDITOR_H

#include "kdepim_export.h"
#include <KPIMTextEdit/TextEdit>

namespace KPIMIdentities {
  class Signature;
}

class KUrl;

namespace KPIM {

class KMeditorPrivate;
class KEMailQuotingHighlighter;

/**
 * The KMeditor class provides a widget to edit and display text,
 * specially geared towards writing e-mails.
 *
 * It offers sevaral additional functions of a KRichTextWidget:
 *
 * @li The ability to us an external editor
 * @li Utility functions like removing whitespace, inserting a file,
 *     adding quotes or rot13'ing the text
 */
class KDEPIM_EXPORT KMeditor : public KPIMTextEdit::TextEdit
{
  Q_OBJECT

  public:


    /**
     * Constructs a KMeditor object
     */
    explicit KMeditor( const QString& text, QWidget *parent = 0 );

    /**
     * Constructs a KMeditor object.
     */
    explicit KMeditor( QWidget *parent = 0 );

    virtual ~KMeditor();

    //Redefine it for each apps
    virtual QString smartQuote( const QString & msg ); //need by kmail

    void setUseExternalEditor( bool use );
    void setExternalEditorPath( const QString & path );
    bool checkExternalEditorFinished();
    void killExternalEditor();

    /**
     * Show the open file dialog and returns the selected URL there.
     * The file dialog has an encoding combobox displayed, and the selected
     * encoding there will be set as the encoding of the URL's fileEncoding().
     */
    KUrl insertFile();

    /**
     * Enables word wrap. Words will be wrapped at the specified column.
     *
     * @param wrapColumn the column where words will be wrapped
     */
    void enableWordWrap( int wrapColumn );

    /**
     * Disables word wrap.
     * Note that words are still wrapped at the end of the editor; no scrollbar
     * will appear.
     */
    void disableWordWrap();

    /**
     * Changes the font of the whole text.
     * Also sets the default font for the document.
     *
     * @param font the font that the whole text will get
     */
    void setFontForWholeText( const QFont &font );

    void setCursorPositionFromStart( unsigned int pos );

    /**
     * @return the line number where the cursor is. This takes word-wrapping
     *         into account. Line numbers start at 0.
     */
    int linePosition();

    /**
     * @return the column numbe where the cursor is.
     */
    int columnNumber();

    /**
     * Reimplemented again to work around a bug (see comment in implementation).
     * FIXME: This is _not_ virtual in the base class
     */
    void ensureCursorVisible();

  public Q_SLOTS:

    void slotAddQuotes();
    void slotRemoveBox();
    void slotAddBox();
    void slotPasteAsQuotation();
    void slotRemoveQuotes();

    /**
     * Warning: This function switches back to plain text mode.
     */
    void slotRot13();

    void slotChangeInsertMode();

  Q_SIGNALS:

    /**
     * Emitted whenever the foucs is lost or gained
     *
     * @param focusGained true if the focus was gained, false when it was lost
     */
    void focusChanged( bool focusGained );

    /**
     * Emitted when the user uses the up arrow in the first line. The application
     * should then put the focus on the widget above the text edit.
     */
    void focusUp();

    void insertModeChanged();

  protected:

    /**
     * Reimplemented to start the external editor and to emit focusUp().
     */
    virtual void keyPressEvent ( QKeyEvent * e );

  private:
    KMeditorPrivate *const d;
    friend class KMeditorPrivate;
    Q_PRIVATE_SLOT( d, void ensureCursorVisibleDelayed() )
    Q_PRIVATE_SLOT( d, void slotEditorFinished( int, QProcess::ExitStatus ) )
};

}

#endif
