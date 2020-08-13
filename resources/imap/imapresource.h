/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef IMAPRESOURCE_H
#define IMAPRESOURCE_H

#include <imapresourcebase.h>

class ImapResource : public ImapResourceBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Imap.Resource")

public:
    explicit ImapResource(const QString &id);
    ~ImapResource() override;

    QDialog *createConfigureDialog(WId windowId) override;

protected:
    QString defaultName() const override;
    QByteArray clientId() const override;

private Q_SLOTS:
    void onConfigurationDone(int result);
};

#endif // IMAPRESOURCE_H
