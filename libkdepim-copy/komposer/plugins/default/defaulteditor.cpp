/**
 * defaulteditor.cpp
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
#include "defaulteditor.h"
#include "core.h"

#include <kgenericfactory.h>
#include <kapplication.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <kaction.h>
#include <kcolordialog.h>
#include <kfiledialog.h>
#include <kinstance.h>
#include <klocale.h>
#include <kstandardaction.h>
#include <kprinter.h>
#include <kfinddialog.h>
#include <kfind.h>
#include <kreplacedialog.h>
#include <kreplace.h>

#include <QTextEdit>
#include <QWidget>

typedef KGenericFactory<DefaultEditor> DefaultEditorFactory;
K_EXPORT_COMPONENT_FACTORY( libkomposer_defaulteditor,
                            DefaultEditorFactory( "komposer_defaulteditor" ) )

DefaultEditor::DefaultEditor( QObject *parent, const char *name, const QStringList &args )
  : Editor( parent, name, args ), m_textEdit( 0 )
{
  setInstance( DefaultEditorFactory::instance() );

  m_textEdit = new QTextEdit( 0 );

  createActions( actionCollection() );

  setXMLFile( "defaulteditorui.rc" );
}

DefaultEditor::~DefaultEditor()
{
}


QWidget*
DefaultEditor::widget()
{
    return m_textEdit;
}

QString
DefaultEditor::text() const
{
  return m_textEdit->text();
}

void
DefaultEditor::setText( const QString &text )
{
  m_textEdit->setText( text );
}

void
DefaultEditor::changeSignature( const QString &sig )
{
  QString text = m_textEdit->text();

  int sigStart = text.lastIndexOf( "-- " );
  QString sigText = QString( "-- \n%1" ).arg( sig );

  text.replace( sigStart, text.length(), sigText );
}

void
DefaultEditor::createActions( KActionCollection *ac )
{
  //
  // File Actions
  //
  (void) KStandardAction::open( this, SLOT(open()), ac );
  (void) KStandardAction::openRecent( this, SLOT(openURL(const KUrl &)), ac );
  (void) KStandardAction::save( this, SLOT(save()), ac );
  (void) KStandardAction::saveAs( this, SLOT(saveAs()), ac );

  //
  // Edit Actions
  //
  KAction *actionUndo = KStandardAction::undo( m_textEdit, SLOT(undo()), ac );
  actionUndo->setEnabled( false );
  connect( m_textEdit, SIGNAL(undoAvailable(bool)),
           actionUndo, SLOT(setEnabled(bool)) );

  KAction *actionRedo = KStandardAction::redo( m_textEdit, SLOT(redo()), ac );
  actionRedo->setEnabled( false );
  connect( m_textEdit, SIGNAL(redoAvailable(bool)),
           actionRedo, SLOT(setEnabled(bool)) );

  KAction *action_cut = KStandardAction::cut( m_textEdit, SLOT(cut()), ac );
  action_cut->setEnabled( false );
  connect( m_textEdit, SIGNAL(copyAvailable(bool)),
           action_cut, SLOT(setEnabled(bool)) );

  KAction *action_copy = KStandardAction::copy( m_textEdit, SLOT(copy()), ac );
  action_copy->setEnabled( false );
  connect( m_textEdit, SIGNAL(copyAvailable(bool)),
           action_copy, SLOT(setEnabled(bool)) );

  (void) KStandardAction::print( this, SLOT(print()), ac );

  (void) KStandardAction::paste( m_textEdit, SLOT(paste()), ac );
  (void) new KAction( i18n( "C&lear" ), 0,
                      m_textEdit, SLOT(removeSelectedText()),
                      ac, "edit_clear" );

  (void) KStandardAction::selectAll( m_textEdit, SLOT(selectAll()), ac );

  //
  // View Actions
  //
  (void) KStandardAction::zoomIn( m_textEdit, SLOT(zoomIn()), ac );
  (void) KStandardAction::zoomOut( m_textEdit, SLOT(zoomOut()), ac );

  //
  // Character Formatting
  //
  m_actionBold = new KToggleAction( i18n("&Bold"), "text_bold", Qt::CTRL+Qt::Key_B,
                                    ac, "format_bold" );
  connect( m_actionBold, SIGNAL(toggled(bool)),
           m_textEdit, SLOT(setBold(bool)) );

  m_actionItalic = new KToggleAction( i18n("&Italic"), "text_italic", Qt::CTRL+Qt::Key_I,
                                      ac, "format_italic" );

  connect( m_actionItalic, SIGNAL(toggled(bool)),
           m_textEdit, SLOT(setItalic(bool) ));

  m_actionUnderline = new KToggleAction( i18n("&Underline"), "text_under", Qt::CTRL+Qt::Key_U,
                                         ac, "format_underline" );

  connect( m_actionUnderline, SIGNAL(toggled(bool)),
           m_textEdit, SLOT(setUnderline(bool)) );

  (void) new KAction( i18n("Text &Color..."), "colorpicker", 0,
                      this, SLOT(formatColor()),
                      ac, "format_color" );

  //
  // Font
  //
  m_actionFont = new KFontAction( i18n("&Font"), 0,
                                 ac, "format_font" );
  connect( m_actionFont, SIGNAL(activated(const QString &)),
           m_textEdit, SLOT(setFamily(const QString &)) );


  m_actionFontSize = new KFontSizeAction( i18n("Font &Size"), 0,
                                          ac, "format_font_size" );
  connect( m_actionFontSize, SIGNAL(fontSizeChanged(int)),
           m_textEdit, SLOT(setPointSize(int)) );

  //
  // Alignment
  //
  m_actionAlignLeft = new KToggleAction( i18n("Align &Left"), "text_left", 0,
                                         ac, "format_align_left" );
  connect( m_actionAlignLeft, SIGNAL(toggled(bool)),
           this, SLOT(setAlignLeft(bool)) );

  m_actionAlignCenter = new KToggleAction( i18n("Align &Center"), "text_center", 0,
                                           ac, "format_align_center" );
  connect( m_actionAlignCenter, SIGNAL(toggled(bool)),
           this, SLOT(setAlignCenter(bool)) );

  m_actionAlignRight = new KToggleAction( i18n("Align &Right"), "text_right", 0,
                                          ac, "format_align_right" );
  connect( m_actionAlignRight, SIGNAL(toggled(bool)),
           this, SLOT(setAlignRight(bool)) );

  m_actionAlignJustify = new KToggleAction( i18n("&Justify"), "text_block", 0,
                                            ac, "format_align_justify" );
  connect( m_actionAlignJustify, SIGNAL(toggled(bool)),
           this, SLOT(setAlignJustify(bool)) );

  m_actionAlignLeft->setExclusiveGroup( "alignment" );
  m_actionAlignCenter->setExclusiveGroup( "alignment" );
  m_actionAlignRight->setExclusiveGroup( "alignment" );
  m_actionAlignJustify->setExclusiveGroup( "alignment" );

  //
  // Tools
  //
  (void) KStandardAction::spelling( this, SLOT(checkSpelling()), ac );

  //
  // Setup enable/disable
  //
  updateActions();

  connect( m_textEdit, SIGNAL(currentFontChanged(const QFont &)),
           this, SLOT( updateFont() ) );
  connect( m_textEdit, SIGNAL(currentFontChanged(const QFont &)),
           this, SLOT(updateCharFmt()) );
  connect( m_textEdit, SIGNAL(cursorPositionChanged(int, int)),
           this, SLOT(updateAligment()) );
}

void
DefaultEditor::updateActions()
{
  updateCharFmt();
  updateAligment();
  updateFont();
}

void
DefaultEditor::updateCharFmt()
{
  m_actionBold->setChecked( m_textEdit->bold() );
  m_actionItalic->setChecked( m_textEdit->italic() );
  m_actionUnderline->setChecked( m_textEdit->underline() );
}

void
DefaultEditor::updateAligment()
{
  int align = m_textEdit->alignment();

  switch ( align ) {
  case AlignRight:
    m_actionAlignRight->setChecked( true );
    break;
  case AlignCenter:
    m_actionAlignCenter->setChecked( true );
    break;
  case AlignLeft:
    m_actionAlignLeft->setChecked( true );
    break;
  case AlignJustify:
    m_actionAlignJustify->setChecked( true );
    break;
  default:
    break;
  }
}

void
DefaultEditor::updateFont()
{
  if ( m_textEdit->pointSize() > 0 )
    m_actionFontSize->setFontSize( m_textEdit->pointSize() );
  m_actionFont->setFont( m_textEdit->family() );
}

void
DefaultEditor::formatColor()
{
  QColor col;

  int s = KColorDialog::getColor( col, m_textEdit->color(), m_textEdit );
  if ( s != QDialog::Accepted )
    return;

  m_textEdit->setColor( col );
}

void
DefaultEditor::setAlignLeft( bool yes )
{
  if ( yes )
    m_textEdit->setAlignment( AlignLeft );
}

void
DefaultEditor::setAlignRight( bool yes )
{
  if ( yes )
    m_textEdit->setAlignment( AlignRight );
}

void
DefaultEditor::setAlignCenter( bool yes )
{
  if ( yes )
    m_textEdit->setAlignment( AlignCenter );
}

void
DefaultEditor::setAlignJustify( bool yes )
{
  if ( yes )
    m_textEdit->setAlignment( AlignJustify );
}

//
// Content Actions
//

bool
DefaultEditor::open()
{
  KUrl url = KFileDialog::getOpenURL();
  if ( url.isEmpty() )
    return false;

  //fixme
  //return openURL( url );
  return true;
}

bool
DefaultEditor::saveAs()
{
  KUrl url = KFileDialog::getSaveURL();
  if ( url.isEmpty() )
    return false;

  //FIXME
  //return KParts::ReadWritePart::saveAs( url );
  return true;
}

void
DefaultEditor::checkSpelling()
{
  QString s;
  if ( m_textEdit->hasSelectedText() )
    s = m_textEdit->selectedText();
  else
    s = m_textEdit->text();

  //KSpell::modalCheck( s );
}

bool
DefaultEditor::print()
{
  return true;
}

#include "defaulteditor.moc"
