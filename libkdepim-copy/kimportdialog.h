/*
    This file is part of libkdepim.

    Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef KIMPORTDIALOG_H
#define KIMPORTDIALOG_H

#include <q3intdict.h>
#include <QStringList>
#include <QSpinBox>
#include <q3ptrvector.h>
#include <q3valuevector.h>
#include <Q3PtrList>

#include <kdialog.h>

class Q3Table;
class Q3ListView;
class Q3ListViewItem;

class KImportDialog;
class KComboBox;

class KImportColumn
{
  public:
    enum { FormatUndefined = 0, FormatPlain, FormatUnquoted, FormatBracketed, FormatLast };

    KImportColumn(KImportDialog *dlg, const QString &header, int count = 0);
    virtual ~KImportColumn() {}

    QString header() const { return m_header; }

    QList<int> formats();
    QString formatName(int format);
    int defaultFormat();

    QString convert();
//    virtual void convert(const QString &value,int format) = 0;
    QString preview(const QString &value,int format);

    void addColId(int i);
    void removeColId(int i);

    QList<int> colIdList();

  protected:

  private:
    int m_maxCount, m_refCount;

    QString m_header;
    QList<int> mFormats;
    int mDefaultFormat;

    QList<int> mColIds;

    KImportDialog *mDialog;
};

class KImportDialog : public KDialog
{
    Q_OBJECT
  public:
    KImportDialog(QWidget* parent);

  public Q_SLOTS:
    bool setFile(const QString& file);
    bool setFile(const KUrl& file);
    QString cell(int row);

    void addColumn(KImportColumn *);

  protected:
    void readFile( int rows = 10 );

    void fillTable();
    void registerColumns();
    int findFormat(int column);

    virtual void convertRow() {}

  protected Q_SLOTS:
    void separatorClicked(int id);
    void formatSelected(Q3ListViewItem* item);
    void headerSelected(Q3ListViewItem* item);
    void assignColumn(Q3ListViewItem *);
    void assignColumn();
    void assignTemplate();
    void removeColumn();
    void applyConverter();
    void tableSelected();
    void slotUrlChanged(const QString & );
    void saveTemplate();

  private:
    void updateFormatSelection(int column);
    void setCellText(int row, int col, const QString& text);

    void setData( int row, int col, const QString &text );
    QString data( int row, int col );

    Q3ListView *mHeaderList;
    QSpinBox *mStartRow;
    QSpinBox *mEndRow;
    Q3Table *mTable;

    KComboBox *mFormatCombo;
    KComboBox *mSeparatorCombo;

    QString mSeparator;
    int mCurrentRow;
    QString mFile;
    Q3IntDict<KImportColumn> mColumnDict;
    Q3IntDict<int> mTemplateDict;
    QMap<int,int> mFormats;
    Q3PtrList<KImportColumn> mColumns;
    Q3PtrVector<Q3ValueVector<QString> > mData;
};

#endif
