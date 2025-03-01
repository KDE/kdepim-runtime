#include "accountsintegrator.h"

AccountsIntegrator::AccountsIntegrator(DavGroupwareResource *parent)
    : QObject(parent)
    , m_resource(parent)
{
}

void AccountsIntegrator::setAccount(const QDBusObjectPath &path)
{
    m_resource->setAccount(path);
}
