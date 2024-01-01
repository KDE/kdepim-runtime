/*
  This file is part of libkldap.

  SPDX-FileCopyrightText: 2003-2009 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KCModule>
namespace KLDAPWidgets
{
class LdapConfigureWidget;
}

class KCMLdap : public KCModule
{
    Q_OBJECT

public:
    explicit KCMLdap(QObject *parent, const KPluginMetaData &data);
    ~KCMLdap() override;

    void load() override;
    void save() override;

private:
    KLDAPWidgets::LdapConfigureWidget *const mLdapConfigureWidget;
};
