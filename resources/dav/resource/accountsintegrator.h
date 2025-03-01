#pragma once

#include <QDBusObjectPath>
#include <QObject>

#include "davgroupwareresource.h"

class AccountsIntegrator : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.Accounts")

public:
    AccountsIntegrator(DavGroupwareResource *parent);

    Q_SCRIPTABLE void setAccount(const QDBusObjectPath &path);

private:
    DavGroupwareResource *m_resource;
};
