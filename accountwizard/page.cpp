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

#include "page.h"
#include <kpagewidgetmodel.h>
#include <kassistantdialog.h>

Page::Page(KAssistantDialog *parent):
    QWidget(parent),
    m_item(0),
    m_parent(parent),
    m_valid(false)
{
}

void Page::setPageWidgetItem(KPageWidgetItem *item)
{
    m_item = item;
    m_parent->setValid(m_item, m_valid);
}

void Page::setValid(bool valid)
{
    if (!m_item) {
        m_valid = valid;
    } else {
        m_parent->setValid(m_item, valid);
    }
}

void Page::nextPage()
{
    m_parent->next();
}

void Page::enterPageBack() {}
void Page::enterPageNext() {}
void Page::leavePageBack() {}
void Page::leavePageNext() {}

void Page::leavePageBackRequested()
{
    emit leavePageBackOk();
}
void Page::leavePageNextRequested()
{
    emit leavePageNextOk();
}

