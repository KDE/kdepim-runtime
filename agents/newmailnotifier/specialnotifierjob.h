/*
   Copyright (C) 2013-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef SPECIALNOTIFIERJOB_H
#define SPECIALNOTIFIERJOB_H

#include <QObject>
#include <Item>
#include <QStringList>
#include <QPixmap>
#include <Akonadi/KMime/MarkAsCommand>

class KJob;

class SpecialNotifierJob : public QObject
{
    Q_OBJECT
public:
    explicit SpecialNotifierJob(const QStringList &listEmails, const QString &path, Akonadi::Item::Id id, QObject *parent = nullptr);
    ~SpecialNotifierJob();

    void setDefaultIconName(const QString &iconName);

Q_SIGNALS:
    void displayDefaultIconNotification(const QString &message);
    void displayNotification(const QPixmap &pixmap, const QString &message);
    void say(const QString &message);

private:
    void slotSearchJobFinished(KJob *job);
    void slotItemFetchJobDone(KJob *);
    void slotMarkAsResult(Akonadi::CommandBase::Result result);
    void slotOpenMail();
    void slotMarkAsRead();
    void slotDeleteMessage();
    void slotActivateNotificationAction(unsigned int index);
    void emitNotification(const QPixmap &pixmap = QPixmap());
    void deleteItemDone(KJob *job);
    QStringList mListEmails;
    QString mDefaultIconName;
    QString mSubject;
    QString mFrom;
    QString mPath;
    Akonadi::Item mItem;
    Akonadi::Item::Id mItemId;
};

#endif // SPECIALNOTIFIERJOB_H
