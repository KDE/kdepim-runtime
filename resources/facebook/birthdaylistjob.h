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
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FACEBOOK_BIRTHDAYLISTJOB_H_
#define FACEBOOK_BIRTHDAYLISTJOB_H_

#include "listjob.h"

class BirthdayListJob : public ListJob
{
    Q_OBJECT

public:
    BirthdayListJob(const Akonadi::Collection &collection, QObject *parent = nullptr);
    ~BirthdayListJob() override;

protected:
    Akonadi::Item handleResponse(const QJsonObject &data) override;
};

#endif
