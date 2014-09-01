/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef PAGE_H
#define PAGE_H

#include <QWidget>

class KAssistantDialog;
class KPageWidgetItem;

class Page : public QWidget
{
    Q_OBJECT
public:
    explicit Page(KAssistantDialog *parent);

    void setPageWidgetItem(KPageWidgetItem *item);

    virtual void enterPageNext();
    virtual void enterPageBack();
    virtual void leavePageNext();
    virtual void leavePageBack();
    virtual void leavePageNextRequested();
    virtual void leavePageBackRequested();

signals:
    Q_SCRIPTABLE void pageEnteredNext();
    Q_SCRIPTABLE void pageEnteredBack();
    Q_SCRIPTABLE void pageLeftNext();
    Q_SCRIPTABLE void pageLeftBack();
    Q_SCRIPTABLE void leavePageNextOk();
    Q_SCRIPTABLE void leavePageBackOk();

public slots:
    Q_SCRIPTABLE void setValid(bool valid);
    Q_SCRIPTABLE void nextPage();

protected:
    KPageWidgetItem *m_item;
    KAssistantDialog *m_parent;

private:
    friend class Dialog;
    bool m_valid;
};

#endif // PAGE_H
