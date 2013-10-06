/*
    Copyright (c) 2013 Mark Gaiser <markg85@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

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
