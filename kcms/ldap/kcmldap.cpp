/*
  This file is part of libkldap.

  SPDX-FileCopyrightText: 2002-2009 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcmldap.h"

#include <QVBoxLayout>

#include <KAboutData>
#include <KLDAPWidgets/LdapConfigureWidget>
#include <KLocalizedString>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(KCMLdap, "kcmldap.json")
KCMLdap::KCMLdap(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , mLdapConfigureWidget(new KLDAPWidgets::LdapConfigureWidget(widget()))
{
    setButtons(KCModule::Apply);
    auto layout = new QVBoxLayout(widget());
    layout->setContentsMargins({});

    layout->addWidget(mLdapConfigureWidget);
    connect(mLdapConfigureWidget, &KLDAPWidgets::LdapConfigureWidget::changed, this, &KCMLdap::setNeedsSave);
}

KCMLdap::~KCMLdap() = default;

void KCMLdap::load()
{
    mLdapConfigureWidget->load();
}

void KCMLdap::save()
{
    mLdapConfigureWidget->save();
}

#include "kcmldap.moc"

#include "moc_kcmldap.cpp"
