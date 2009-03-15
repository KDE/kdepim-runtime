#include "dao.h"

DAO::DAO()
{
  doc = new XmlDocument();
}

void DAO::openXmlDocument(const QString &path)
{
  doc->loadFile(path);
}  

Collection DAO::getCollectionByRemoteId(const QString &rid)
{
  return doc->collectionByRemoteId(rid);
}

Item DAO::getItemByRemoteId(const QString &rid)
{
  return doc->itemByRemoteId(rid, true);
}
