/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SPECIALNOTIFIERJOB_H
#define SPECIALNOTIFIERJOB_H

#include <Akonadi/KMime/MarkAsCommand>
#include <Item>
#include <QObject>
#include <QPixmap>
#include <QStringList>

class KJob;

class SpecialNotifierJob : public QObject
{
    Q_OBJECT
public:
    explicit SpecialNotifierJob(const QStringList &listEmails, const QString &path, Akonadi::Item::Id id, QObject *parent = nullptr);
    ~SpecialNotifierJob();

    void setDefaultIconName(const QString &iconName);

Q_SIGNALS:
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
    const QStringList mListEmails;
    QString mDefaultIconName;
    QString mSubject;
    QString mFrom;
    const QString mPath;
    Akonadi::Item mItem;
    const Akonadi::Item::Id mItemId;
};

#endif // SPECIALNOTIFIERJOB_H
