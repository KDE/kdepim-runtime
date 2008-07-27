/**
 * kmeditor.h
 *
 * Copyright 2007 Laurent Montel <montel@kde.org>
 * Copyright 2008 Thomas McGuire <thomas.mcguire@gmx.net>
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
#include <KRichTextWidget>

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
 * @li Quote highlighting
 * @li The ability to us an external editor
 * @li Signature handling
 * @li Better spellcheck support
 * @li Utility functions like removing whitespace, inserting a file,
 *     adding quotes or rot13'ing the text
 *
 * Do not call any spellchecking related function of KRichTextWidget, use the function
 * of this class instead.
 */
class KDEPIM_EXPORT KMeditor : public KRichTextWidget
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

    virtual ~KMeditor();

    virtual void changeHighlighterColors(KEMailQuotingHighlighter*);

    //Redefine it for each apps
    virtual QString quotePrefixName() const; //define by kmail
    virtual QString smartQuote( const QString & msg ); //need by kmail

    void setUseExternalEditor( bool use );
    void setExternalEditorPath( const QString & path );

    /// FIXME: Huh? This is not virtual in the base classes and thus never called
    void paste();

    KUrl insertFile( const QStringList &encodingLst, QString &encodingStr );

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
     *
     * Rich text mode will be enabled if the signature is in inlined HTML format.
     *
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
     *
     * Use the other insertSignature() function if possible, as it has support
     * for separators and does HTML detection automatically.
     *
     * Rich text mode will be enabled if @p isHtml is true.
     *
     * @param placement defines where in the textedit the signature should be
     *                  inserted.
     * @param isHtml defines whether the signature should be inserted as text or html
     */
    void insertSignature( const QString &signature, Placement placement = End,
                          bool isHtml = false );

    /**
     * Replaces all occurrences of the old signature with the new signature.
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

    /**
     * @return true if highlighting of missspelled words is enabled
     * @ref slotToggleSpellChecking
     */
    bool isSpellCheckingEnabled() const;

    /**
     * Toggle spellcheck highlighting on or off.
     * Spellchecking is disabled by default.
     *
     * @ref spellCheckingEnabled
     */
    void toggleSpellChecking( bool on );

    /**
     * This shows the standard spellcheck config dialog of Sonnet and saves
     * the settings in the given config file.
     * The language is automatically changed when the user changes the
     * language in the config dialog.
     *
     * @param configFileName the name of the config file where the settings will
     *                       be loaded from and saved to.
     */
    void showSpellConfigDialog( const QString &configFileName );

    /**
     * Returns the text of the editor as plain text, with linebreaks inserted
     * where word-wrapping occurred.
     */
    QString toWrappedPlainText() const;

    /**
     * Reimplemented again to work around a bug (see comment in implementation).
     * FIXME: This is _not_ virtual in the base class
     */
    void ensureCursorVisible();

  public Q_SLOTS:

    /**
     * Sets the spell checking language, as used by the highlighter and the
     * spellcheck dialog.
     *
     * @param language the language to use for spelling. This has to be one of
     *                 the values returned by Sonnet::Speller::availableLanguages(),
     *                 which are also available in KPIM::DictionaryCombBox.
     */
    void setSpellCheckLanguage( const QString &language );

    void slotAddQuotes();
    void slotRemoveBox();
    void slotAddBox();

    /**
     * Shows the spell checkdialog, where the user can perform spell checking
     * on the whole document.
     * The language is automatically changed when the user changes the
     * language in the spellcheck dialog.
     */
    void slotCheckSpelling();

    void slotPasteAsQuotation();
    void slotRemoveQuotes();

    /**
     * Warning: This function switches back to plain text mode.
     */
    void slotRot13();

    void slotChangeInsertMode();

  Q_SIGNALS:

    /**
     * Emitted when the user chooses a different spellcheck language,
     * either in the config dialog or in the checking dialog.
     * Also emitted after setSpellCheckLanguage().
     *
     * @param language the language which the user selected
     */
    void spellcheckLanguageChanged( const QString &language );

    /**
     * Emitted whenever the foucs is lost or gained
     *
     * @param focusGained true if the focus was gained, false when it was lost
     */
    void focusChanged( bool focusGained );

    void pasteImage();
    void focusUp();
    void overwriteModeText();

  protected:

    // HACK: (KDE5: fix this properly in KTextEdit by cleaning up the API and
    //              making functions virtual)
    // Make all spellcheck-related functions of KTextEdit protected. They should
    // not be called from the outside. The problem is that qoute highlighting only
    // works when spell checking is enabled (as this is done in one hightlighter,
    // KEMailQuotingHighlighter), so we have to pretend to KTextEdit that spell
    // checking is enabled the whole time.
    // These functions are not virtual, so we can't simply reimplement them.
    // And no, having two separate QSyntaxHighlighters does not work either,
    // one of them disables the highlighting of the other.
    void setCheckSpellingEnabled( bool );
    bool checkSpellingEnabled() const;
    void setSpellCheckingLanguage( const QString & );
    void setHighlighter( Sonnet::Highlighter * );
    Sonnet::Highlighter* highlighter() const;
    void setSpellCheckingConfigFileName( const QString & );

    /**
     * Reimplemented to create our own highlighter which does quote and
     * spellcheck highlighting
     */
    virtual void createHighlighter();

    virtual void dragEnterEvent( QDragEnterEvent *e );
    virtual void dragMoveEvent( QDragMoveEvent *e );
    virtual bool eventFilter( QObject *o, QEvent *e );
    virtual void keyPressEvent( QKeyEvent *e );

    /*
     * Redefine it to allow to create context menu for spell word list
     */
    virtual void contextMenuEvent( QContextMenuEvent* );

    /**
     * Handles drag enter/move event for dragEnterEvent() and dragMoveEvent()
     */
    void handleDragEvent( QDragMoveEvent *e );

    /**
     * Reimplemented to block rich text pastes in plain text mode.
     */
    virtual void insertFromMimeData ( const QMimeData * source );

  private:
    KMeditorPrivate *const d;
    friend class KMeditorPrivate;
    Q_PRIVATE_SLOT( d, void ensureCursorVisibleDelayed() )
    Q_PRIVATE_SLOT( d, void slotEditorFinished( int, QProcess::ExitStatus ) )
};

}

#endif
