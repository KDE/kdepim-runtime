#include "resource.h"

void Resource::load(const QString &resourceName, const QString &path)
{
  const AgentType = AgentManager::self()->type(resourceName);

  mResource = resourceName;
  mPath = path;
}

void Resource::load()
{
  load(resource, path);
}
