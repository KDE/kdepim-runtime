#include "generichandler.h"
#include "googleresource.h"

GenericHandler::GenericHandler(GoogleResource* resource)
    : m_resource(resource)
{
}

GenericHandler::~GenericHandler() = default;

void GenericHandler::emitReadyStatus()
{
    Q_EMIT status(Akonadi::AgentBase::Idle, i18nc("@info:status", "Ready"));
}

#include "moc_generichandler.cpp"
