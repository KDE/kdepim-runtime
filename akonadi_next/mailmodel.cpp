
#include "mailmodel.h"

#include <kmime/kmime_message.h>

#include <boost/shared_ptr.hpp>

#include <KDebug>

typedef boost::shared_ptr<KMime::Message> MessagePtr;

using namespace Akonadi;

class MailModelPrivate
{
public:
  MailModelPrivate(MailModel *model)
    : q_ptr(model)
  {
    m_collectionHeaders << "Folder" << "Count";
    m_itemHeaders << "Subject" << "From" << "Date";
  }
  Q_DECLARE_PUBLIC(MailModel)
  MailModel *q_ptr;

  QStringList m_itemHeaders;
  QStringList m_collectionHeaders;

};

MailModel::MailModel(Session *session, Monitor *monitor, QObject *parent)
  : EntityTreeModel(session, monitor, parent), d_ptr(new MailModelPrivate(this))
{

}

MailModel::~MailModel()
{
   delete d_ptr;
}

QVariant MailModel::getData(Item item, int column, int role) const
{
  const MessagePtr mail = item.payload<MessagePtr>();
  if (role == Qt::DisplayRole)
  {
    switch (column)
    {
    case 0:
      return mail->subject()->asUnicodeString();
    case 1:
      return mail->from()->asUnicodeString();
    case 2:
      return mail->date()->asUnicodeString();
    }
  } else if (role == Qt::ToolTipRole)
  {
    QString d;
    d.append(QString("Subject: %1\n").arg(mail->subject()->asUnicodeString()));
    d.append(QString("From: %1\n").arg(mail->from()->asUnicodeString()));
    d.append(QString("Date: %1\n").arg(mail->date()->asUnicodeString()));
    return d;
  }
  return EntityTreeModel::getData(item, column, role);
}

QVariant MailModel::getData(Collection collection, int column, int role) const
{
  if (role == Qt::DisplayRole)
  {
    switch (column)
    {
    case 0:
      return EntityTreeModel::getData(collection, column, role);
    case 1:
      return rowCount(EntityTreeModel::indexForCollection(collection));
    default:
      // Return a QString to pass modeltest.
      return QString();
  //     return QVariant();
    }
  }
  return EntityTreeModel::getData(collection, column, role);
}

int MailModel::columnCount(const QModelIndex &index) const
{
  Q_UNUSED(index);
  return 3;
}

QVariant MailModel::getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const
{
  Q_D(const MailModel);

  if (orientation == Qt::Horizontal)
  {
    if (headerSet == EntityTreeModel::CollectionTreeHeaders)
    {
      if (role == Qt::DisplayRole)
      {
        if (section >= d->m_collectionHeaders.size() )
          return QVariant();
        return d->m_collectionHeaders.at(section);
      }
    } else if (headerSet == EntityTreeModel::ItemListHeaders)
    {
      if (role == Qt::DisplayRole)
      {
        if (section >= d->m_itemHeaders.size() )
          return QVariant();
        return d->m_itemHeaders.at(section);
      }
    }
  }
  return EntityTreeModel::getHeaderData(section, orientation, role, headerSet);
}
