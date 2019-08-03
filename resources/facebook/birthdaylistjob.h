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

#ifndef FACEBOOK_BIRTHDAYLISTJOB_H_
#define FACEBOOK_BIRTHDAYLISTJOB_H_

#include <KJob>

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>

#include <KCalendarCore/Event>

class QUrl;
namespace KIO {
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

#endif
