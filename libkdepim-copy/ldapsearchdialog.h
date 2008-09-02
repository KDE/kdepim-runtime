/* ldapsearchdialogimpl.h - LDAP access
 *      Copyright (C) 2002 Klar�vdalens Datakonsult AB
 *
 *      Author: Steffen Hansen <hansen@kde.org>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef LDAPSEARCHDIALOG_H
#define LDAPSEARCHDIALOG_H

#include <QtCore/QList>

#include <ldapclient.h>
#include <kdialog.h>
#include <klineedit.h>

class KComboBox;

class QCheckBox;
class Q3ListView;
class QPushButton;

namespace KPIM {

class KDEPIM_EXPORT LdapSearchDialog : public KDialog
{
  Q_OBJECT

  public:
    explicit LdapSearchDialog( QWidget *parent, const char *name = 0 );
    ~LdapSearchDialog();

    bool isOK() const
    {
      return mIsOK;
    }

    void restoreSettings();

    void setSearchText( const QString &text )
    {
      mSearchEdit->setText( text );
    }

    QString selectedEMails() const;

  signals:
    void addresseesAdded();

  protected slots:
    void slotAddResult( const KPIM::LdapClient &client, const KLDAP::LdapObject &obj );
    void slotSetScope( bool rec );
    void slotStartSearch();
    void slotStopSearch();
    void slotSearchDone();
    void slotError( const QString & );
    virtual void slotButtonClicked( int button );

  protected:
    virtual void closeEvent( QCloseEvent * );

  private:
    void saveSettings();

    QString makeFilter( const QString &query, const QString &attr, bool startsWith );

    void cancelQuery();

    int mNumHosts;
    QList<KPIM::LdapClient *>mLdapClientList;
    bool mIsOK;
    KComboBox *mFilterCombo;
    KComboBox *mSearchType;
    KLineEdit *mSearchEdit;

    QCheckBox *mRecursiveCheckbox;
    Q3ListView *mResultListView;
    QPushButton *mSearchButton;
};

}

#endif
