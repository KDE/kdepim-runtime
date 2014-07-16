/*
 * Copyright (C) 2014  Sandro Knauß <knauss@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AUTOCONFIGKOLABMAIL_H
#define AUTOCONFIGKOLABMAIL_H

#include "ispdb.h"

class AutoconfigKolabMail :  public Ispdb
{
    Q_OBJECT
public:
    /** Constructor */
    explicit AutoconfigKolabMail(QObject *parent = 0);

    virtual void startJob(const KUrl &url);

private slots:
    void slotResult(KJob *);
};

#endif // AUTOCONFIGKOLABMAIL_H
