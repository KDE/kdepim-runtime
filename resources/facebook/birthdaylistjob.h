/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <KJob>

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <KCalendarCore/Event>

class QUrl;
namespace KIO
{
class StoredTransferJob;
}

class BirthdayListJob : public KJob
{
    Q_OBJECT

public:
    BirthdayListJob(const QString &identifier, const Akonadi::Collection &collection, QObject *parent);
    ~BirthdayListJob() override;

    void start() override;

    QVector<Akonadi::Item> items() const;

private:
    KIO::StoredTransferJob *createGetJob(const QUrl &url) const;
    void emitError(const QString &errorText);

    void fetchFacebookEventsPage();
    QUrl findBirthdayIcalLink(const QByteArray &data);
    void fetchBirthdayIcal(const QUrl &url);
    void processEvent(const KCalendarCore::Event::Ptr &event);

    Akonadi::Collection mCollection;
    QVector<Akonadi::Item> mItems;
    QString mCookies;
    QString mIdentifier;
};

