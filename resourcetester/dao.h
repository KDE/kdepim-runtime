#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <akonadi/xml/xmldocument.h>

using namespace Akonadi;

class DAO : public QObject
{
  public:
    DAO();
  
  private:
    XmlDocument *doc;

  public Q_SLOTS:
    void openXmlDocument(const QString &path);
    Collection getCollectionByRemoteId(const QString &rid);
    Item getItemByRemoteId(const QString &rid);
};
