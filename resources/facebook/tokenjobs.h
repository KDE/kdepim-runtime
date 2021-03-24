/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <KJob>

class TokenJob : public KJob
{
    Q_OBJECT
public:
    explicit TokenJob(const QString &identifier, QObject *parent);
    ~TokenJob() override;

    void start() override;

protected:
    void emitError(const QString &text);

    virtual void doStart() = 0;
    QString mIdentifier;
};

class LoginJob : public TokenJob
{
    Q_OBJECT
public:
    explicit LoginJob(const QString &identifier, QObject *parent);
    ~LoginJob() override;

    QString token() const;

protected:
    void doStart() override;
    void fetchUserInfo();
};

class LogoutJob : public TokenJob
{
    Q_OBJECT
public:
    explicit LogoutJob(const QString &identifier, QObject *parent);
    ~LogoutJob() override;

protected:
    void doStart() override;
};

class GetTokenJob : public TokenJob
{
    Q_OBJECT
public:
    explicit GetTokenJob(const QString &identifier, QObject *parent);
    ~GetTokenJob() override;

    QString token() const;
    QString userName() const;
    QString userId() const;
    QByteArray cookies() const;

    void start() override;

protected:
    void doStart() override;
};

