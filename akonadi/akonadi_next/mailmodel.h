
#ifndef MAILMODEL_H
#define MAILMODEL_H

#include "entitytreemodel.h"

#include "akonadi_next_export.h"

using namespace Akonadi;

class MailModelPrivate;

class AKONADI_NEXT_EXPORT MailModel : public EntityTreeModel
{
  Q_OBJECT
public:

  MailModel(Session *session, Monitor *monitor, QObject *parent = 0);
  virtual ~MailModel();

  virtual QVariant getData(Item item, int column, int role=Qt::DisplayRole) const;

  virtual QVariant getData(Collection collection, int column, int role=Qt::DisplayRole) const;

  virtual int columnCount(const QModelIndex &index = QModelIndex()) const;

  virtual QVariant getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const;

private:
    Q_DECLARE_PRIVATE(MailModel)
    MailModelPrivate *d_ptr;
};

#endif
