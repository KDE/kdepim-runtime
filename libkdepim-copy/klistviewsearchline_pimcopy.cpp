/* This file is part of the KDE libraries
   Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "klistviewsearchline_pimcopy.h"

#include <klistview.h>
#include <kdebug.h>

namespace KPIM {

class KListViewSearchLine::KListViewSearchLinePrivate
{
public:
    KListViewSearchLinePrivate() :
        listView(0),
        caseSensitive(false),
        activeSearch(false),
        keepParentsVisible(true) {}
    
    KListView *listView;
    bool caseSensitive;
    bool activeSearch;
    bool keepParentsVisible;
    QString search;
    QValueList<int> searchColumns;
    QValueList<QListViewItem *> parents;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

KListViewSearchLine::KListViewSearchLine(QWidget *parent, KListView *listView, const char *name) :
    KLineEdit(parent, name)
{
    d = new KListViewSearchLinePrivate;

    d->listView = listView;

    // Delete this lineedit if the list view it's attached to is deleted.

    connect(listView, SIGNAL(destroyed()), this, SLOT(deleteLater()));

    connect(this, SIGNAL(textChanged(const QString &)),
	    this, SLOT(updateSearch(const QString &)));

    connect(listView, SIGNAL(itemAdded(QListViewItem *)),
	    this, SLOT(itemAdded(QListViewItem *)));
}

KListViewSearchLine::~KListViewSearchLine()
{
    delete d;
}

bool KListViewSearchLine::caseSensitive() const
{
    return d->caseSensitive;
}

QValueList<int> KListViewSearchLine::searchColumns() const
{
    return d->searchColumns;
}

bool KListViewSearchLine::keepParentsVisible() const
{
    return d->keepParentsVisible;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void KListViewSearchLine::updateSearch(const QString &s)
{
    d->search = s.isNull() ? text() : s;
    checkItem(d->listView->firstChild());
}

void KListViewSearchLine::setCaseSensitive(bool cs)
{
    d->caseSensitive = cs;
}

void KListViewSearchLine::setKeepParentsVisible(bool v)
{
    d->keepParentsVisible = v;
}

void KListViewSearchLine::setSearchColumns(const QValueList<int> &columns)
{
    d->searchColumns = columns;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

bool KListViewSearchLine::itemMatches(const QListViewItem *item, const QString &s) const
{
    if(s.isEmpty())
        return true;

    // If the search column list is populated, search just the columns
    // specifified.  If it is empty default to searching all of the columns.

    if(!d->searchColumns.isEmpty()) {
        QValueList<int>::ConstIterator it = d->searchColumns.begin(); 
        for(; it != d->searchColumns.end(); ++it) {
            if(*it < d->listView->columns() &&
               item->text(*it).find(s, 0, d->caseSensitive) >= 0)
            {
                return true;
            }
        }
    }
    else {
        for(int i = 0; i < d->listView->columns(); i++) {
            if(item->text(i).find(s, 0, d->caseSensitive) >= 0)
                return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void KListViewSearchLine::itemAdded(QListViewItem *item) const
{
    item->setVisible(itemMatches(item, text()));
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void KListViewSearchLine::checkItem(QListViewItem *item)
{
    if(!item)
        return;

    if(itemMatches(item, d->search)) {
        if(d->keepParentsVisible) {
            QValueList<QListViewItem *>::Iterator it = d->parents.begin();
            for(; it != d->parents.end(); ++it)
                (*it)->setVisible(true);
        }
        item->setVisible(true);
    }
    else
        item->setVisible(false);

    if(item->firstChild()) {
        d->parents.append(item);
        checkItem(item->firstChild());
        d->parents.remove(d->parents.fromLast());
    }

    checkItem(item->nextSibling());
}

}
#include "klistviewsearchline_pimcopy.moc"
