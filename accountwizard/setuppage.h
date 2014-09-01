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

#ifndef SETUPPAGE_H
#define SETUPPAGE_H

#include "page.h"
#include "ui_setuppage.h"

class QStandardItemModel;

class SetupPage : public Page
{
    Q_OBJECT
public:
    explicit SetupPage(KAssistantDialog *parent);
    virtual void enterPageNext();

    enum MessageType {
        Success,
        Info,
        Error
    };
    void addMessage(MessageType type, const QString &msg);
    void setStatus(const QString &msg);
    void setProgress(int percent);

private slots:
    void detailsClicked();

private:
    Ui::SetupPage ui;
    QStandardItemModel *m_msgModel;
};

#endif
