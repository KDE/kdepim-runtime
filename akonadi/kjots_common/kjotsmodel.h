
#ifndef KJOTSMODEL_H
#define KJOTSMODEL_H

#include "entitytreemodel.h"

namespace Akonadi
{
class Monitor;
class Session;
}

using namespace Akonadi;

/**
 * A wrapper QObject making some book and page properties available to Grantlee.
 */
class KJotsEntity : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString title READ title)
  Q_PROPERTY(QString content READ content)
  Q_PROPERTY(bool isBook READ isBook)
  Q_PROPERTY(bool isPage READ isPage)
  Q_PROPERTY(QVariantList entities READ entities)

public:
  KJotsEntity(const QModelIndex &index, QObject *parent = 0);

  bool isBook();
  bool isPage();

  QString title();

  QString content();

  QVariantList entities();

private:
  QPersistentModelIndex m_index;
};

class KJotsModel : public EntityTreeModel
{
  Q_OBJECT
public:
  KJotsModel(Session *session, Monitor *monitor, QObject *parent = 0);
  virtual ~KJotsModel();

  enum KJotsRoles
  {
    GrantleeObjectRole = EntityTreeModel::UserRole
  };

  QVariant data(const QModelIndex &index, int role) const;
};

#endif

