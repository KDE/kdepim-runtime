/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etebaseadapter.h"

#include "etesync_debug.h"

QString QStringFromCharPtr(const CharPtr &str)
{
    if (str.get() == nullptr) {
        return QString();
    }
    const QString ret = QString::fromUtf8(str.get());
    return ret;
}

EtebaseClientPtr etebase_client_new(const QString &client_name, const QString &server_url)
{
    return EtebaseClientPtr(etebase_client_new(charArrFromQString(client_name), charArrFromQString(server_url)));
}

EtebaseAccountPtr etebase_account_login(const EtebaseClient *client, const QString &username, const QString &password)
{
    return EtebaseAccountPtr(etebase_account_login(client, charArrFromQString(username), charArrFromQString(password)));
}
