/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <KCompositeJob>

#include <Akonadi/Item>

class QJsonObject;

class ListJob : public KCompositeJob
{
    Q_OBJECT

public:
    explicit ListJob(const QString &identifier, const Akonadi::Collection &col, QObject *parent = nullptr);
    ~ListJob() override;

    Akonadi::Collection collection() const;

    void start() override;

protected:
    void setRequest(const QString &endpoint, const QStringList &fields = {}, const QMap<QString, QString> &queries = {});

    virtual Akonadi::Item handleResponse(const QJsonObject &data) = 0;

    void emitError(const QString &errorString);

    QString mIdentifier;

Q_SIGNALS:
    void itemsAvailable(KJob *self, const Akonadi::Item::List &items, QPrivateSignal);

private Q_SLOTS:
    void tokenJobResult(KJob *job);

    void onGraphResponseReceived(KJob *job);

private:
    void sendRequest(const QUrl &url);

    Akonadi::Collection mCollection;
    QString mEndpoint;
    QStringList mFields;
    QMap<QString, QString> mQueries;
};

