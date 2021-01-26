/*
  This file is part of libkldap.

  SPDX-FileCopyrightText: 2003-2009 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KCMLDAP_H
#define KCMLDAP_H

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
    KLDAP::LdapConfigureWidget *mLdapConfigureWidget = nullptr;
};

#endif
