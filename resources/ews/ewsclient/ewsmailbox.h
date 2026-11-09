/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QList>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QString>

class QXmlStreamReader;

class EwsMailboxPrivate;

namespace KMime
{
namespace Types
{
class Mailbox;
}
}

class EwsMailbox
{
public:
    typedef QList<EwsMailbox> List;

    EwsMailbox();
    explicit EwsMailbox(QXmlStreamReader &reader);
    EwsMailbox(const EwsMailbox &other);
    EwsMailbox(EwsMailbox &&other);
    virtual ~EwsMailbox();

    EwsMailbox &operator=(const EwsMailbox &other);
    EwsMailbox &operator=(EwsMailbox &&other);

    bool isValid() const;
    QString name() const;
    QString email() const;
    QString emailWithName() const;
    operator KMime::Types::Mailbox() const;

protected:
    QSharedDataPointer<EwsMailboxPrivate> d;
};

Q_DECLARE_METATYPE(EwsMailbox)
Q_DECLARE_METATYPE(EwsMailbox::List)
