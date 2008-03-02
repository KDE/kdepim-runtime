/**
 * kmeditor.h
 *
 * Copyright (C)  2007  Laurent Montel <montel@kde.org>
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

#ifndef KMEDITOR_H
#define KMEDITOR_H

#include "kdepim_export.h"
#include <ktextedit.h>
#include <qtextformat.h>

class KFindDialog;
class KUrl;

namespace KPIMIdentities {
  class Signature;
}

namespace KPIM {

class KMeditorPrivate;
class KEMailQuotingHighlighter;
class KDEPIM_EXPORT KMeditor : public KTextEdit
{
  Q_OBJECT

  public:

    /**
     * Describes the placement of a text which is to be inserted into this
     * textedit
     */
    enum Placement { Start,    ///< The text is placed at the start of the textedit
                     End,      ///< The text is placed at the end of the textedit
                     AtCursor  ///< The text is placed at the current cursor position
                   };

    /**
     * Constructs a KMeditor object
     */
    explicit KMeditor( const QString& text, QWidget *parent = 0 );

    /**
     * Constructs a KMeditor object.
     */
    explicit KMeditor( QWidget *parent = 0 );

    ~KMeditor();

    virtual void createHighlighter();

    virtual void changeHighlighterColors(KEMailQuotingHighlighter*);

    //Redefine it for each apps
    virtual QString quotePrefixName() const; //define by kmail
    virtual QString smartQuote( const QString & msg ); //need by kmail

    void setUseExternalEditor( bool use );
    void setExternalEditorPath( const QString & path );

    void dragEnterEvent( QDragEnterEvent *e );
    void dragMoveEvent( QDragMoveEvent *e );
    void keyPressEvent( QKeyEvent * e );

    void paste();

    void switchTextMode(bool useHtml);

    KUrl insertFile( const QStringList &encodingLst, QString &encodingStr );

    void wordWrapToggled( bool on );

    void setColor( const QColor& );
    void setFont( const QFont& );

    bool checkExternalEditorFinished();
    void killExternalEditor();
    void setCursorPositionFromStart( unsigned int pos );

    int linePosition();
    int columnNumber();
    void setCursorPosition( int linePos, int columnPos );

    /**
     * Inserts the signature @p sig into the textedit.
     * The cursor position is preserved.
     * A leading or trailing newline is also added automatically, depending on
     * the placement.
     * For undo/redo, this is treated as one operation.
     * @param placement defines where in the textedit the signature should be
     *                  inserted.
     * @param addSeparator if true, the separator '-- \n' will be added in front
     *                     of the signature
     */
    void insertSignature( const KPIMIdentities::Signature &sig,
                          Placement placement = End, bool addSeparator = true );

    /**
     * Inserts the signature @p sig into the textedit.
     * The cursor position is preserved.
     * A leading or trailing newline is also added automatically, depending on
     * the placement.
     * For undo/redo, this is treated as one operation.
     * A separator is not added.
     * @param placement defines where in the textedit the signature should be
     *                  inserted.
     * @param isHtml defines whether the signature should be inserted as text or html
     */
    void insertSignature( const QString &signature, Placement placement = End, bool isHtml = false );

    /**
     * Replaces all occurences of the old signature with the new signature.
     * Text in quotes will be ignored.
     * For undo/redo, this is treated as one operation.
     * If the old signature is empty, nothing is done.
     * If the new signature is empty, the old signature including the
     * separator is removed.
     *
     * @param oldSig the old signature, which will be replaced
     * @param newSig the new signature
     */
    void replaceSignature( const KPIMIdentities::Signature &oldSig,
                           const KPIMIdentities::Signature &newSig );

    /**
     * Cleans the whitespace of the edit's text.
     * Adjacent tabs and spaces will be converted to a single space.
     * Trailing whitespace will be removed.
     * More than 2 newlines in a row will be changed to 2 newlines.
     * Text in quotes or text inside of the given signature will not be
     * cleaned.
     * For undo/redo, this is treated as one operation.
     *
     * @param sig text inside this signature will not be cleaned
     */
    void cleanWhitespace( const KPIMIdentities::Signature &sig );

  public Q_SLOTS:
    void slotAddQuotes();
    void slotRemoveBox();
    void slotAddBox();
    void slotAlignLeft();
    void slotAlignCenter();
    void slotAlignRight();
    void slotChangeParagStyle( QTextListFormat::Style _style );
    void slotFontFamilyChanged( const QString &f );
    void slotFontSizeChanged( int size );
    void slotPasteAsQuotation();
    void slotRemoveQuotes();
    void slotRot13();
    void slotTextBold( bool _b );
    void slotTextItalic( bool _b );
    void slotTextUnder( bool _b );
    void slotTextColor();


    void slotChangeInsertMode();
  Q_SIGNALS:
    void pasteImage();
    void focusUp();
    void overwriteModeText();
    void highlighterCreated();

  protected:

    bool eventFilter( QObject* o, QEvent* e );
    void init();

    /*
     * Redefine it to allow to create context menu for spell word list
     */
    virtual void contextMenuEvent( QContextMenuEvent* );

  private:

    KMeditorPrivate *const d;
    Q_PRIVATE_SLOT( d, void addSuggestion( const QString&, const QStringList& ) )
};

}

#endif
