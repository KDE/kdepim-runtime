#ifndef _NAO_FREEDESKTOPICON_H_
#define _NAO_FREEDESKTOPICON_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

#include "nao/symbol.h"
namespace Nepomuk {
namespace NAO {
/**
 * Represents a desktop icon as defined in the FreeDesktop Icon 
 * Naming Standard 
 */
class FreeDesktopIcon : public NAO::Symbol
{
public:
    FreeDesktopIcon(Nepomuk::SimpleResource* res)
      : NAO::Symbol(res), m_res(res)
    {}

    virtual ~FreeDesktopIcon() {}

    /**
     * Get property http://www.semanticdesktop.org/ontologies/2007/08/15/nao#iconName. 
     * Defines a name for a FreeDesktop Icon as defined in the FreeDesktop 
     * Icon Naming Standard 
     */
    QStringList iconNames() const {
        QStringList value;
        foreach(const QVariant& v, m_res->property(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#iconName", QUrl::StrictMode)))
            value << v.value<QString>();
        return value;
    }

    /**
     * Set property http://www.semanticdesktop.org/ontologies/2007/08/15/nao#iconName. 
     * Defines a name for a FreeDesktop Icon as defined in the FreeDesktop 
     * Icon Naming Standard 
     */
    void setIconNames(const QStringList& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        QVariantList values;
        foreach(const QString& v, value)
            values << v;
        m_res->setProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#iconName", QUrl::StrictMode), values);
    }

    /**
     * Add value to property http://www.semanticdesktop.org/ontologies/2007/08/15/nao#iconName. 
     * Defines a name for a FreeDesktop Icon as defined in the FreeDesktop 
     * Icon Naming Standard 
     */
    void addIconName(const QString& value) {
        m_res->addProperty(Soprano::Vocabulary::RDF::type(), resourceType());
        m_res->addProperty(QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#iconName", QUrl::StrictMode), value);
    }

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/08/15/nao#FreeDesktopIcon", QUrl::StrictMode); }

private:
    Nepomuk::SimpleResource* m_res;
};
}
}

#endif
