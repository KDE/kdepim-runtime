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

#include "dynamicpage.h"

#include <QDebug>

#include <QUiLoader>
#include <QFile>
#include <qboxlayout.h>
#include <qscrollarea.h>

DynamicPage::DynamicPage(const QString &uiFile, KAssistantDialog *parent) : Page(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    setLayout(layout);

#ifdef KDEPIM_MOBILE_UI
    // for mobile ui we put the page into a scroll area in case it's too big
    QScrollArea *pageParent = new QScrollArea(this);
    pageParent->setFrameShape(QFrame::NoFrame);
    pageParent->setWidgetResizable(true);
    layout->addWidget(pageParent);
#else
    QWidget *pageParent = this;
#endif

    QUiLoader loader;
    QFile file(uiFile);
    file.open(QFile::ReadOnly);
    qDebug() << uiFile;
    m_dynamicWidget = loader.load(&file, pageParent);
    file.close();

#ifdef KDEPIM_MOBILE_UI
    pageParent->setWidget(m_dynamicWidget);
#else
    layout->addWidget(m_dynamicWidget);
#endif

    setValid(true);
}

QObject *DynamicPage::widget() const
{
    return m_dynamicWidget;
}

