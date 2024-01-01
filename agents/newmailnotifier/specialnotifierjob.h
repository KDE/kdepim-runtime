/*
   SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <Akonadi/MarkAsCommand>
#include <QObject>
#include <QPixmap>
#include <QStringList>

class KJob;

class SpecialNotifierJob : public QObject
{
    Q_OBJECT
public:
    struct SpecialNotificationInfo {
        QStringList listEmails;
        QString path;
        Akonadi::Item::Id itemId;
        QString defaultIconName;
        QString resourceName;
    };
    explicit SpecialNotifierJob(const SpecialNotificationInfo &info, QObject *parent = nullptr);
    ~SpecialNotifierJob() override;

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
    void emitNotification(const QPixmap &pixmap = QPixmap());
    void deleteItemDone(KJob *job);
    void slotReplyMessage();
    QString mSubject;
    QString mFrom;
    Akonadi::Item mItem;
    const SpecialNotificationInfo mSpecialNotificationInfo;
};
Q_DECLARE_TYPEINFO(SpecialNotifierJob::SpecialNotificationInfo, Q_RELOCATABLE_TYPE);
