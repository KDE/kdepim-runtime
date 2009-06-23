/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef NOTIFICATIONMODEL
#define NOTIFICATIONMODEL

#include <akonadi/private/notificationmessage_p.h>

#include <QAbstractItemModel>
#include <QDateTime>

class NotificationModel : public QAbstractItemModel
{
  Q_OBJECT
  public:
    NotificationModel( QObject *parent );

    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    bool isEnabled() const { return m_enabled; }

  public slots:
    void clear();
    void setEnabled( bool enable ) { m_enabled = enable; }

  private slots:
    void slotNotify( const Akonadi::NotificationMessage::List &msgs );

  private:
    struct NotificationBlock {
      NotificationBlock( const Akonadi::NotificationMessage::List &msgs );
      Akonadi::NotificationMessage::List msgs;
      QDateTime timestamp;
    };
    QList<NotificationBlock> m_data;
    bool m_enabled;
};

#endif
