#ifndef KRES_AKONADI_IDARBITERBASE_H
#define KRES_AKONADI_IDARBITERBASE_H

#include <QHash>
#include <QSet>

class IdArbiterBase
{
  public:
    virtual ~IdArbiterBase();

    QString arbitrateOriginalId( const QString &originalId );

    QString removeArbitratedId( const QString &arbitratedId );

    QString mapToOriginalId( const QString &arbitratedId ) const;

    void clear();

  protected:
    typedef QSet<QString> IdSet;
    typedef QHash<QString, QString> IdMapping;
    typedef QHash<QString, IdSet> IdSetMapping;
    IdSetMapping mOriginalToArbitrated;
    IdMapping mArbitratedToOriginal;

  protected:
    IdSet mapToArbitratedIds( const QString &originalId ) const;

    virtual QString createArbitratedId() const = 0;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
