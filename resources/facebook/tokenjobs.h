/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TOKENJOBS_H_
#define TOKENJOBS_H_

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

#endif
