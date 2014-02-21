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

#include "todoitemdelegate.h"
#include "todomodel.h"

#include <KGlobal>
#include <KLocale>
#include <KMacroExpanderBase>

#include <QDateTime>
#include <QHash>
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>

TodoItemDelegate::TodoItemDelegate(QObject* parent, QString normal, QString todo, QString noDueDate, int dtFormat, QString dtString)
    : QStyledItemDelegate(parent),
    m_normal(normal),
    m_todo(todo),
    m_noDueDate(noDueDate),
    m_dateString(dtString),
    m_dateFormat(dtFormat)
{
}

TodoItemDelegate::~TodoItemDelegate()
{
}

QString TodoItemDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale);
    QMap<QString, QVariant> data = value.toMap();
    QString mainCategory = data["mainCategory"].toString();

    int itemType = data["itemType"].toInt();
    switch (itemType) {
        case TodoModel::HeaderItem:
            return  KMacroExpander::expandMacros(data["title"].toString(), titleHash(data));
            break;
        case TodoModel::NormalItem:
        case TodoModel::BirthdayItem:
        case TodoModel::AnniversaryItem:
            if (m_categoryFormats.contains(mainCategory))
                return KMacroExpander::expandMacros(m_categoryFormats.value(mainCategory), eventHash(data));
            else
                return KMacroExpander::expandMacros(m_normal, eventHash(data));
            break;
        case TodoModel::TodoItem:
            if (data["hasDueDate"].toBool() == false)
                return KMacroExpander::expandMacros(m_noDueDate, todoHash(data));
            else
                return KMacroExpander::expandMacros(m_todo, todoHash(data));
            break;
        default:
            break;
    }

    return QString();
}

void TodoItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    painter->save();

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    QVariant value = index.data();
    QBrush bgBrush = qvariant_cast<QBrush>(index.data(Qt::BackgroundRole));
    QBrush fgBrush = qvariant_cast<QBrush>(index.data(Qt::ForegroundRole));
    painter->setClipRect( opt.rect );
    painter->setBackgroundMode(Qt::OpaqueMode);
    painter->setBackground(Qt::transparent);
    painter->setBrush(bgBrush);

    if (bgBrush.style() != Qt::NoBrush) {
        QPen bgPen;
        bgPen.setColor(bgBrush.color().darker(250));
        bgPen.setStyle(Qt::SolidLine);
        bgPen.setWidth(1);
        painter->setPen(bgPen);
        painter->drawRoundedRect(opt.rect.x(), opt.rect.y(), opt.rect.width() - bgPen.width(), opt.rect.height() - bgPen.width(), 3.0, 3.0);
    }

    QTextDocument doc;
    doc.setDocumentMargin(3);
    doc.setDefaultStyleSheet("* {color: " + fgBrush.color().name() + ";}");
    doc.setHtml("<html><qt></head><meta name=\"qrichtext\" content=\"1\" />" + displayText(value, QLocale::system()) + "</qt></html>");
    QAbstractTextDocumentLayout::PaintContext context;
    doc.setPageSize( opt.rect.size());
    painter->translate(opt.rect.x(), opt.rect.y());
    doc.documentLayout()->draw(painter, context);
    painter->restore();
}

QSize TodoItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant value = index.data();
    QTextDocument doc;
    doc.setDocumentMargin(3);
    doc.setHtml("<html><qt></head><meta name=\"qrichtext\" content=\"1\" />" + displayText(value, QLocale::system()) + "</qt></html>");
    doc.setTextWidth(option.rect.width());
    return doc.size().toSize();
}

QHash<QString, QString> TodoItemDelegate::titleHash(QMap<QString, QVariant> data) const
{
    QHash<QString,QString> dataHash;
    dataHash.insert("date", formattedDate(data["date"]));
    dataHash.insert("weekday", data["date"].toDateTime().toString("dddd"));

    return dataHash;
}

QHash<QString, QString> TodoItemDelegate::eventHash(QMap<QString, QVariant> data) const
{
    QHash<QString,QString> dataHash;
    ulong s;
    dataHash.insert("startDate", formattedDate(data["startDate"]));
    dataHash.insert("endDate", formattedDate(data["endDate"]));
    dataHash.insert("startTime", KGlobal::locale()->formatTime(data["startDate"].toTime()));
    dataHash.insert("endTime", KGlobal::locale()->formatTime(data["endDate"].toTime()));
    s = data["startDate"].toDateTime().secsTo(data["endDate"].toDateTime());
    dataHash.insert("duration", KGlobal::locale()->prettyFormatDuration(s * 1000));
    dataHash.insert("summary", data["summary"].toString());
    dataHash.insert("description", data["description"].toString());
    dataHash.insert("location", data["location"].toString());
    dataHash.insert("yearsSince", data["yearsSince"].toString());
    dataHash.insert("collectionName", data["collectionName"].toString());
    dataHash.insert("mainCategory", data["mainCategory"].toString());
    dataHash.insert("categories", data["categories"].toStringList().join(", "));
    dataHash.insert("contactName", data["contactName"].toString());
    dataHash.insert("tab", "\t");

    return dataHash;
}

QHash<QString, QString> TodoItemDelegate::todoHash(QMap<QString, QVariant> data) const
{
    QHash<QString,QString> dataHash;
    dataHash.insert("startDate", formattedDate(data["startDate"]));
    dataHash.insert("startTime", KGlobal::locale()->formatTime(data["startDate"].toTime()));
    dataHash.insert("dueDate", formattedDate(data["dueDate"]));
    dataHash.insert("dueTime", KGlobal::locale()->formatTime(data["dueDate"].toTime()));
    dataHash.insert("summary", data["summary"].toString());
    dataHash.insert("description", data["description"].toString());
    dataHash.insert("location", data["location"].toString());
    dataHash.insert("collectionName", data["collectionName"].toString());
    dataHash.insert("mainCategory", data["mainCategory"].toString());
    dataHash.insert("categories", data["categories"].toStringList().join(", "));
    dataHash.insert("percent", QString::number(data["percent"].toInt()));
    dataHash.insert("tab", "\t");

    return dataHash;
}

void TodoItemDelegate::setCategoryFormats(QMap<QString, QString> formats)
{
	m_categoryFormats = formats;
}

QString TodoItemDelegate::formattedDate(const QVariant &dtTime) const
{
    QString date;
    if (dtTime.toDateTime().isValid()) {
        switch (m_dateFormat) {
            case ShortDateFormat:
                date = KGlobal::locale()->formatDate(dtTime.toDate(), KLocale::ShortDate);
                break;
            case LongDateFormat:
                date = KGlobal::locale()->formatDate(dtTime.toDate(), KLocale::LongDate);
                break;
            case FancyShortDateFormat:
                date = KGlobal::locale()->formatDate(dtTime.toDate(), KLocale::FancyShortDate);
                break;
            case FancyLongDateFormat:
                date = KGlobal::locale()->formatDate(dtTime.toDate(), KLocale::FancyLongDate);
                break;
            case CustomDateFormat:
                date = dtTime.toDate().toString(m_dateString);
                break;
        }
    }

    return date;
}

void TodoItemDelegate::settingsChanged(QString normal, QString todo, QString noDueDate, int format, QString customString)
{
    m_normal = normal;
    m_todo= todo;
    m_noDueDate = noDueDate;
    m_dateFormat = format;
    m_dateString = customString;
}
