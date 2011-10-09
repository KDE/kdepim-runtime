/*
 *  Copyright (c) 2011 Grégory Oestreicher <greg@kamago.net>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include "ui_searchdialog.h"

#include <KDialog>

class KJob;
class QStandardItemModel;

class SearchDialog : public KDialog
{
  Q_OBJECT

  public:
    SearchDialog( QWidget *parent = 0 );
    virtual ~SearchDialog();

    bool useDefaultCredentials() const;

    void setUsername( const QString &user );
    QString username() const;

    void setPassword( const QString &password );
    QString password() const;

    QStringList selection() const;

  private Q_SLOTS:
    void checkUserInput();
    void search();
    void onSearchJobFinished( KJob *job );
    void onCollectionsFetchJobFinished( KJob *job );

  private:
    Ui::SearchDialog mUi;
    QStandardItemModel *mModel;
    int mSubJobCount;
};

#endif // SEARCHDIALOG_H