/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SPECIALNOTIFIERJOB_H
#define SPECIALNOTIFIERJOB_H


#include <QObject>
#include <Akonadi/Item>
#include <QStringList>
class KJob;

class SpecialNotifierJob : public QObject
{
    Q_OBJECT
public:
    explicit SpecialNotifierJob(const QStringList &listEmails, const QString &path, Akonadi::Item::Id id, QObject *parent = 0);
    ~SpecialNotifierJob();

Q_SIGNALS:
    void displayNotification(const QPixmap &pixmap, const QString &message);

private Q_SLOTS:
    void slotSearchJobFinished( KJob *job );
    void slotItemFetchJobDone(KJob*);
    void slotOpenMail();
private:
    void emitNotification(const QPixmap &pixmap);
    QStringList mListEmails;
    QString mSubject;
    QString mFrom;
    QString mPath;
    Akonadi::Item::Id mItemId;
};

#endif // SPECIALNOTIFIERJOB_H
