/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef GMAILPASSWORDREQUESTER_H
#define GMAILPASSWORDREQUESTER_H

#include <imap/passwordrequesterinterface.h>

#include <LibKGAPI2/Types>

class GmailResource;
class GmailPasswordRequester : public PasswordRequesterInterface
{
  Q_OBJECT

public:
    GmailPasswordRequester(GmailResource *resource, QObject *parent);
    virtual ~GmailPasswordRequester();

    virtual void requestPassword(RequestType request = StandardRequest, const QString &serverError = QString());

private:
    bool isTokenExpired(const QString &serverError);
    QString encodePassword(const QString &accountName, const QString &accessToken) const;

private Q_SLOTS:
    void onAuthFinished(const KGAPI2::AccountPtr &account, bool userRejected);

private:
    GmailResource *mResource;
};

#endif // GMAILPASSWORDREQUESTER_H
