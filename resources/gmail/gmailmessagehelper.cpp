/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "gmailmessagehelper.h"
#include "gmailretrieveitemstask.h"

GmailMessageHelper::GmailMessageHelper(const Akonadi::Collection &collection, ResourceTask *currentTask)
    : MessageHelper()
    , mCollection(collection)
    , mTask(currentTask)
{
}

Akonadi::Item GmailMessageHelper::createItemFromMessage(KMime::Message::Ptr message, const qint64 uid, const qint64 size, const QList<KIMAP::MessageAttribute> &attrs, const QList<QByteArray> &flags, const KIMAP::FetchJob::FetchScope &scope, bool &ok) const
{
    Akonadi::Item item = MessageHelper::createItemFromMessage(message, uid, size, attrs, flags, scope, ok);
    if (!ok) {
        qWarning() << "Failed to read imap message";
        return item;
    }

    Q_FOREACH (const KIMAP::MessageAttribute &attr, attrs) {
        if (attr.first == "X-GM-LABELS") {
            if (mTask) {
                QVector<QByteArray> labels;

                QByteArray labelStr = attr.second.toByteArray();
                int lastPos = 0;
                bool isQuoted = false;
                int i = 0;
                while (i < labelStr.size()) {
                    if (labelStr[i] == '(') {
                        lastPos = i;
                        i++;
                        continue;
                    }

                    if (labelStr[i] == '\"') {
                        lastPos = i;
                        isQuoted = true;
                        i++;
                    }

                    if (isQuoted) {
                        while (labelStr[i] != '\"') {
                            if (i == labelStr.size()) {
                                // huh? Broken string?
                                break;
                            }
                            i++;
                        }
                        QByteArray mid = labelStr.mid(lastPos + 1, i - lastPos - 1).trimmed();
                        if (!mid.isEmpty() && mid != "\\\\All") {
                            labels.append(mid.replace("\\\\", "\\"));
                        }
                        isQuoted = false;
                        lastPos = i;
                    } else {
                        if (labelStr[i] == ' ' || labelStr[i] == ')') {
                            QByteArray mid = labelStr.mid(lastPos + 1, i - lastPos - 1).trimmed();
                            if (!mid.isEmpty() && mid != "\\\\All") {
                                labels.append(mid.replace("\\\\", "\\"));
                            }
                            lastPos = i;
                        }
                    }
                    ++i;
                }
                if (!labels.isEmpty()) {
                    GmailRetrieveItemsTask *task = qobject_cast<GmailRetrieveItemsTask *>(mTask);
                    Q_ASSERT(task);
                    task->linkItem(item.remoteId(), labels);
                }
            }
        } else if (attr.first == "X-GM-THRID") {
            // TODO: Store thread information
        } else if (attr.first == "X-GM-MSGID") {
            item.setGid(attr.second.toString());
        }
    }

    return item;
}
