#pragma once

#include <QDBusObjectPath>
#include <QObject>

#include "googleresource.h"

class AccountsIntegrator : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Accounts")

public:
    AccountsIntegrator(GoogleResource *parent);

    Q_SCRIPTABLE void setAccount(const QDBusObjectPath &path);

private:
    GoogleResource *m_resource;
};
