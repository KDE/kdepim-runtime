/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *   Copyright (C) 2014 by Heena Mahour <heena393@gmail.com>
 */

#ifndef TODOITEMDELEGATE_H
#define TODOITEMDELEGATE_H

#include <QStyledItemDelegate>

class TodoItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    TodoItemDelegate(QObject* parent, QString normal, QString todo, QString noDueDate, int dtFormat, QString dtString);
    ~TodoItemDelegate();

    QString displayText(const QVariant &value, const QLocale &locale)  const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void settingsChanged(QString normal, QString todo, QString noDueDate, int format, QString customString);
    void setCategoryFormats(QMap<QString, QString>);

private:
    QHash<QString, QString> titleHash(QMap<QString, QVariant>) const;
    QHash<QString, QString> eventHash(QMap<QString, QVariant>) const;
    QHash<QString, QString> todoHash(QMap<QString, QVariant>) const;
    QMap<QString, QString> m_categoryFormats;
    QString formattedDate(const QVariant &dtTime) const;

    QString m_normal, m_todo, m_noDueDate, m_dateString;
    int m_dateFormat;
};

#endif

