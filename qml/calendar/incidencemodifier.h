#ifndef INCIDENCEMODIFIER_H
#define INCIDENCEMODIFIER_H

#include <QObject>
#include <Akonadi/Item>
#include <KCalCore/Event>

class IncidenceModifier : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString summary READ summary WRITE setSummary NOTIFY summaryChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)

public:
    explicit IncidenceModifier(QObject *parent = 0);

    Q_INVOKABLE void setId(qint64 id);
    Q_INVOKABLE void save();

    QString summary();
    void setSummary(const QString &summary);

    QString description();
    void setDescription(const QString &description);


    
public slots:
    void slotItemsReceived (const Akonadi::Item::List &items);
    void slotAboutToStart();

signals:
    void incidenceLoaded(qint64 id);
    void summaryChanged();
    void descriptionChanged();
    
private:

    qint64 m_id;
    Akonadi::Item m_item;
    KCalCore::Incidence::Ptr m_incidence;

};

#endif // INCIDENCEMODIFIER_H
