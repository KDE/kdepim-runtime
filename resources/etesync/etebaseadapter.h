/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETEBASEADAPTER_H
#define ETEBASEADAPTER_H

#include <etebase.h>

#include <QString>
#include <memory>
#include <vector>

#define charArrFromQString(str) ((str).isNull() ? nullptr : qUtf8Printable((str)))

struct EtebaseDeleter
{
    void operator()(EtebaseClient *ptr)
    {
        etebase_client_destroy(ptr);
    }

    void operator()(EtebaseAccount *ptr)
    {
        etebase_account_destroy(ptr);
    }

    void operator()(char *ptr)
    {
        std::free(ptr);
    }
};

using EtebaseClientPtr = std::unique_ptr<EtebaseClient, EtebaseDeleter>;
using EtebaseAccountPtr = std::unique_ptr<EtebaseAccount, EtebaseDeleter>;
using CharPtr = std::unique_ptr<char, EtebaseDeleter>;

QString QStringFromCharPtr(const CharPtr &str);

EtebaseClient *etebase_client_new(const QString &client_name, const QString &server_url);

EtebaseAccount *etebase_account_login(const EtebaseClient *client, const QString &username, const QString &password);

#endif
