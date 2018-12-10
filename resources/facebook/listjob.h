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

#ifndef FACEBOOK_LISTJOB_H_
#define FACEBOOK_LISTJOB_H_

#include <KCompositeJob>

#include <AkonadiCore/Item>

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
    void itemsAvailable(KJob *self, const Akonadi::Item::List & items, QPrivateSignal);

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

#endif
