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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#ifndef KIMPORTDIALOG_H
#define KIMPORTDIALOG_H

#include <qintdict.h>
#include <qstringlist.h>
#include <qspinbox.h>
#include <qptrvector.h>
#include <qvaluevector.h>

#include <kdialogbase.h>

class QTable;
class QListView;

class KImportDialog;
class KComboBox;

class KImportColumn
{
  public:
    enum { FormatUndefined = 0, FormatPlain, FormatUnquoted, FormatBracketed, FormatLast };

    KImportColumn(KImportDialog *dlg, const QString &header, int count = 0);
    virtual ~KImportColumn() {}

    QString header() const { return m_header; }

    QValueList<int> formats();
    QString formatName(int format);
    int defaultFormat();

    QString convert();
//    virtual void convert(const QString &value,int format) = 0;
    QString preview(const QString &value,int format);

    void addColId(int i);
    void removeColId(int i);

    QValueList<int> colIdList();

  protected:

  private:
    int m_maxCount, m_refCount;

    QString m_header;
    QValueList<int> mFormats;
    int mDefaultFormat;
    
    QValueList<int> mColIds;
    
    KImportDialog *mDialog;
};

class KImportDialog : public KDialogBase
{
    Q_OBJECT
  public:
    KImportDialog(QWidget* parent);

  public slots:
    bool setFile(const QString& file);

    QString cell(uint row);

    void addColumn(KImportColumn *);

  protected:
    void readFile( int rows = 10 );
  
    void fillTable();
    void registerColumns();
    int findFormat(int column);

    virtual void convertRow() {};

  protected slots:
    void separatorClicked(int id);
    void formatSelected(QListViewItem* item);
    void headerSelected(QListViewItem* item);
    void assignColumn(QListViewItem *);
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

    void setData( uint row, uint col, const QString &text );
    QString data( uint row, uint col );

    QListView *mHeaderList;
    QSpinBox *mStartRow;
    QSpinBox *mEndRow;
    QTable *mTable;

    KComboBox *mFormatCombo;
    KComboBox *mSeparatorCombo;

    QString mSeparator;
    int mCurrentRow;
    QString mFile;
    QIntDict<KImportColumn> mColumnDict;
    QIntDict<uint> mTemplateDict;
    QMap<int,int> mFormats;
    QPtrList<KImportColumn> mColumns;
    QPtrVector<QValueVector<QString> > mData;
};

#endif
