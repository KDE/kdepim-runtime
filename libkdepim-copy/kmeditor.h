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

#include <ktextedit.h>
#include <kdepim_export.h>

class KDEPIM_EXPORT KMeditor : public KTextEdit
{
    Q_OBJECT

  public:
    /**
     * Constructs a KTextEdit 
     */
    explicit KMeditor( const QString& text, QWidget *parent = 0 );

    /**
     * Constructs a KMeditor object.
     */
    explicit KMeditor( QWidget *parent = 0 );

    ~KMeditor();

    virtual void createHighlighter();

    void setUseExternalEditor( bool use ); 
    void setExternalEditorPath( const QString & path );

    void dragEnterEvent( QDragEnterEvent *e );
    void dragMoveEvent( QDragMoveEvent *e );
    void keyPressEvent ( QKeyEvent * e );

    virtual void dropEvent( QDropEvent *e );

    void paste();

    void find();
    void replace();

  protected:
    void init();

  signals:
    void pasteImage();

  private:
    class Private;
    Private *const d;
    Q_PRIVATE_SLOT( d, void addSuggestion(const QString&,const QStringList&) )
    Q_PRIVATE_SLOT( d, void slotHighlight( const QString &, int, int ) )
    Q_PRIVATE_SLOT( d, void slotFindNext() )
};

#endif 

