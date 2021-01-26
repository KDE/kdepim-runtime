/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009, 2010 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_ITEMMODIFYJOB_H
#define AKONADI_FILESTORE_ITEMMODIFYJOB_H

#include "job.h"

#include <item.h>

namespace Akonadi
{
namespace FileStore
{
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT ItemModifyJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    explicit ItemModifyJob(const Item &item, AbstractJobSession *session = nullptr);

    ~ItemModifyJob() override;

    void setIgnorePayload(bool ignorePayload);

    bool ignorePayload() const;

    Item item() const;

    const QSet<QByteArray> &parts() const;
    void setParts(const QSet<QByteArray> &parts);

    bool accept(Visitor *visitor) override;

private:
    void handleItemModified(const Akonadi::Item &item);

private:
    class Private;
    Private *const d;
};
}
}

#endif
