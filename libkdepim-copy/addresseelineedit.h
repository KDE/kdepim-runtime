/*
  This file is part of libkdepim.

  Copyright (c) 2002 Helge Deller <deller@gmx.de>
  Copyright (c) 2002 Lubos Lunak <llunak@suse.cz>
  Copyright (c) 2001,2003 Carsten Pfeiffer <pfeiffer@kde.org>
  Copyright (c) 2001 Waldo Bastian <bastian@kde.org>
  Copyright (c) 2004 Daniel Molkentin <danimo@klaralvdalens-datakonsult.se>
  Copyright (c) 2004 Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef KDEPIM_ADDRESSEELINEEDIT_H
#define KDEPIM_ADDRESSEELINEEDIT_H

#include "kmailcompletion.h"
#include "kdepim_export.h"

#include <kabc/addressee.h>

#include <KLineEdit>

class QDropEvent;
class QEvent;
class QKeyEvent;
class QMenu;
class QMouseEvent;
class QObject;
class QTimer;

namespace KPIM {
  class LdapSearch;
  struct LdapResult;
  typedef QList<LdapResult> LdapResultList;
  typedef QMap< QString, QPair<int,int> > CompletionItemsMap;
}

namespace KPIM {

class KDEPIM_EXPORT AddresseeLineEdit : public KLineEdit
{
  Q_OBJECT

  public:
    explicit AddresseeLineEdit( QWidget *parent, bool useCompletion = true );
    virtual ~AddresseeLineEdit();

    virtual void setFont( const QFont & );

  public Q_SLOTS:
    void cursorAtEnd();
    void enableCompletion( bool enable );
    /** Reimplemented for stripping whitespace after completion */
    virtual void setText( const QString & txt );

  protected Q_SLOTS:
    virtual void loadContacts();
    void slotIMAPCompletionOrderChanged();
  protected:
    void addContact( const KABC::Addressee &, int weight, int source = -1 );
    virtual void keyPressEvent( QKeyEvent * );
    /**
     * Reimplemented for smart insertion of email addresses.
     * Features:
     * - Automatically adds ',' if necessary to separate email addresses
     * - Correctly decodes mailto URLs
     * - Recognizes email addresses which are protected against address
     *   harvesters, i.e. "name at kde dot org" and "name(at)kde.org"
     */
    virtual void insert( const QString &text );
    /** Reimplemented for smart insertion of pasted email addresses. */
    virtual void paste();
    /** Reimplemented for smart insertion with middle mouse button. */
    virtual void mouseReleaseEvent( QMouseEvent *e );
    /** Reimplemented for smart insertion of dragged email addresses. */
    virtual void dropEvent( QDropEvent *e );
    void doCompletion( bool ctrlT );

    /**
     * Reimplemented for subclass access to menu
     */
    QMenu *createStandardContextMenu();

    /**
     * Re-implemented for internal reasons.  API not affected.
     *
     * See QLineEdit::contextMenuEvent().
     */
    virtual void contextMenuEvent( QContextMenuEvent * );

    /**
     * Adds the name of a completion source to the internal list of
     * such sources and returns its index, such that that can be used
     * for insertion of items associated with that source.
     */
    int addCompletionSource( const QString & );

    /** return whether we are using sorted or weighted display */
    static KCompletion::CompOrder completionOrder();

  private Q_SLOTS:
    void slotCompletion();
    void slotPopupCompletion( const QString & );
    void slotReturnPressed( const QString & );
    void slotStartLDAPLookup();
    void slotLDAPSearchData( const KPIM::LdapResultList & );
    void slotEditCompletionOrder();
    void slotUserCancelled( const QString & );

  private:
    virtual bool eventFilter( QObject *o, QEvent *e );
    void init();
    void startLoadingLDAPEntries();
    void stopLDAPLookup();

    void setCompletedItems( const QStringList &items, bool autoSuggest );
    void addCompletionItem( const QString &string, int weight, int source,
                            const QStringList *keyWords=0 );
    QString completionSearchText( QString & );
    const QStringList getAdjustedCompletionItems( bool fullSearch );
    void updateSearchString();

    QString m_previousAddresses;
    QString m_searchString;
    bool m_useCompletion;
    bool m_completionInitialized;
    bool m_smartPaste;
    bool m_addressBookConnected;
    bool m_lastSearchMode;
    bool m_searchExtended; //has \" been added?

    //QMap<QString, KABC::Addressee> m_contactMap;

    static bool s_addressesDirty;
    static KMailCompletion *s_completion;
    static CompletionItemsMap *s_completionItemMap;
    static QTimer *s_LDAPTimer;
    static KPIM::LdapSearch *s_LDAPSearch;
    static QString *s_LDAPText;
    static AddresseeLineEdit *s_LDAPLineEdit;
    static QStringList *s_completionSources;

    class AddresseeLineEditPrivate;
    AddresseeLineEditPrivate *d;
};

}

#endif
