/*
  This file is part of libkldap.

  SPDX-FileCopyrightText: 2002-2009 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcmldap_p.h"

#include <QVBoxLayout>

#include <KAboutData>
#include <KPluginFactory>
#include <KLocalizedString>
#include <KLDAP/LdapConfigureWidget>

K_PLUGIN_CLASS_WITH_JSON(KCMLdap, "kcmldap.json")

KCMLdap::KCMLdap(QWidget *parent, const QVariantList &)
    : KCModule(parent)
{
    setButtons(KCModule::Apply);
    KAboutData *about = new KAboutData(QStringLiteral("kcmldap"),
                                       i18n("kcmldap"),
                                       QString(),
                                       i18n("LDAP Server Settings"),
                                       KAboutLicense::LGPL,
                                       i18n("(c) 2009 - 2010 Tobias Koenig"));
    about->addAuthor(i18n("Tobias Koenig"), QString(), QStringLiteral("tokoe@kde.org"));
    setAboutData(about);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    mLdapConfigureWidget = new KLDAP::LdapConfigureWidget(this);
    layout->addWidget(mLdapConfigureWidget);

    connect(mLdapConfigureWidget, &KLDAP::LdapConfigureWidget::changed, this, qOverload<bool>(&KCMLdap::changed));
}

KCMLdap::~KCMLdap()
{
}

void KCMLdap::load()
{
    mLdapConfigureWidget->load();
}

void KCMLdap::save()
{
    mLdapConfigureWidget->save();
}

#include "kcmldap.moc"
