/*
    This file is part of libkdepim.
    Copyright (c) 2002 Helge Deller <deller@gmx.de>
                  2002 Lubos Lunak <llunak@suse.cz>
                  2001,2003 Carsten Pfeiffer <pfeiffer@kde.org>
                  2001 Waldo Bastian <bastian@kde.org>
                  2004 Daniel Molkentin <danimo@klaralvdalens-datakonsult.se>
                  2004 Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>

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

#ifndef ADDRESSEELINEEDIT_H
#define ADDRESSEELINEEDIT_H

#include <qobject.h>
#include <q3ptrlist.h>
#include <qtimer.h>
#include <qpair.h>

//Added by qt3to4:
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QDropEvent>

#include <kabc/addressee.h>

#include "clicklineedit.h"
#include "kcompletion.h"
#include <dcopobject.h>
#include <kdepimmacros.h>

class QMenu;

class KConfig;

namespace KPIM {
class LdapSearch;
class LdapResult;
typedef QList<LdapResult> LdapResultList;
typedef QMap< QString, QPair<int,int> > CompletionItemsMap;
}

namespace KPIM {

class KDE_EXPORT AddresseeLineEdit : public ClickLineEdit, public DCOPObject
{
  K_DCOP
  Q_OBJECT

  public:
    AddresseeLineEdit( QWidget* parent, bool useCompletion = true,
                     const char *name = 0L);
    virtual ~AddresseeLineEdit();

    virtual void setFont( const QFont& );

  public slots:
    void cursorAtEnd();
    void enableCompletion( bool enable );
    /** Reimplemented for stripping whitespace after completion */
    virtual void setText( const QString& txt );

  protected slots:
    virtual void loadContacts();
  protected:
    void addContact( const KABC::Addressee&, int weight, int source = -1 );
    virtual void keyPressEvent( QKeyEvent* );
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
    virtual QMenu *createStandardContextMenu();

    /**
     * Adds the name of a completion source to the internal list of 
     * such sources and returns its index, such that that can be used
     * for insertion of items associated with that source.
     */
    int addCompletionSource( const QString& );
    
    /** return whether we are using sorted or weighted display */
    static KCompletion::CompOrder completionOrder();

  k_dcop:
    // Connected to the DCOP signal
    void slotIMAPCompletionOrderChanged();

  private slots:
    void slotCompletion();
    void slotPopupCompletion( const QString& );
    void slotStartLDAPLookup();
    void slotLDAPSearchData( const KPIM::LdapResultList& );
    void slotEditCompletionOrder();
    void slotUserCancelled( const QString& );

  private:
    virtual bool eventFilter(QObject *o, QEvent *e);
    void init();
    void startLoadingLDAPEntries();
    void stopLDAPLookup();

    void setCompletedItems( const QStringList& items, bool autoSuggest );
    void addCompletionItem( const QString& string, int weight, int source );
    QString completionSearchText( QString& );
    const QStringList getAdjustedCompletionItems( bool fullSearch );

    QString m_previousAddresses;
    QString m_searchString;
    bool m_useCompletion;
    bool m_completionInitialized;
    bool m_smartPaste;
    bool m_addressBookConnected;

    //QMap<QString, KABC::Addressee> m_contactMap;

    static bool s_addressesDirty;
    static KCompletion *s_completion;
    static CompletionItemsMap* s_completionItemMap;
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
