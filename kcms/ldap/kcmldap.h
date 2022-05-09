/*
  This file is part of libkldap.

  SPDX-FileCopyrightText: 2003-2009 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2022 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KCModule>

namespace KLDAP
{
class LdapConfigureWidget;
}

class KCMLdap : public KCModule
{
    Q_OBJECT

public:
    explicit KCMLdap(QWidget *parent, const QVariantList &args);
    ~KCMLdap() override;

    void load() override;
    void save() override;

private:
    KLDAP::LdapConfigureWidget *const mLdapConfigureWidget;
};
