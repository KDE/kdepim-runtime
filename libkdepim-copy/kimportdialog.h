#ifndef KIMPORTDIALOG_H
#define KIMPORTDIALOG_H
// $Id$

#include <qintdict.h>
#include <qstringlist.h>
#include <qptrvector.h>
#include <qvaluevector.h>

#include <kdialogbase.h>

class QTable;
class QListView;

class KImportDialog;

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
    void removeColumn();
    void applyConverter();
    void tableSelected();
    void slotUrlChanged(const QString & );

  private:
    void updateFormatSelection(int column);
    void setCellText(int row, int col, const QString& text);

    void setData( uint row, uint col, const QString &text );
    QString data( uint row, uint col );

    QTable *mTable;
    QListView *mHeaderList;
    QListView *mFormatList;
    
    QString mSeparator;
    QString mFile;
    QIntDict<KImportColumn> mColumnDict;
    QMap<int,int> mFormats;
    int mCurrentRow;
    QPtrList<KImportColumn> mColumns;

    QPtrVector<QValueVector<QString> > mData;
};

#endif
