// $Id$
//
// Generic CSV import. Please do not add application specific code to this
// class. Application specific code should go to a subclass provided by the
// application using this dialog.

#include <qfile.h>
#include <qtable.h>
#include <qlineedit.h>
#include <qtextstream.h>
#include <qbuttongroup.h>
#include <qlistview.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qregexp.h>

#include <kdebug.h>
#include <kurlrequester.h>
#include <klocale.h>
#include <klineedit.h>
#include <kprogress.h>
#include <kapplication.h>

#include "kimportdialog.h"
#include "kimportdialog.moc"

KImportColumn::KImportColumn(KImportDialog *dlg,const QString &header, int count)
    : m_maxCount(count),
      m_refCount(0),
      m_header(header),
      mDialog(dlg)
{
  mFormats.append(FormatPlain);
  mFormats.append(FormatUnquoted);
//  mFormats.append(FormatBracketed);

  mDefaultFormat = FormatUnquoted;

  mDialog->addColumn(this);
}

QValueList<int> KImportColumn::formats()
{
  return mFormats;
}

QString KImportColumn::formatName(int format)
{
  switch (format) {
    case FormatPlain:
      return i18n("Plain");
    case FormatUnquoted:
      return i18n("Unquoted");
    case FormatBracketed:
      return i18n("Bracketed");
    default:
      return i18n("Undefined");
  }
}

int KImportColumn::defaultFormat()
{
  return mDefaultFormat;
}

QString KImportColumn::preview(const QString &value, int format)
{
  if (format == FormatBracketed) {
    return "(" + value + ")";
  } else if (format == FormatUnquoted) {
    if (value.left(1) == "\"" && value.right(1) == "\"") {
      return value.mid(1,value.length()-2);
    } else {
      return value;
    }
  } else {
    return value;
  }
}

void KImportColumn::addColId(int id)
{
  mColIds.append(id);
}

void KImportColumn::removeColId(int id)
{
  mColIds.remove(id);
}

QString KImportColumn::convert()
{
  QValueList<int>::ConstIterator it = mColIds.begin();
  if (it == mColIds.end()) return "";
  else return mDialog->cell(*it);
}


class ColumnItem : public QListViewItem {
  public:
    ColumnItem(KImportColumn *col,QListView *parent) : QListViewItem(parent), mColumn(col)
    {
      setText(0,mColumn->header());
    }

    KImportColumn *column() { return mColumn; }

  private:
    KImportColumn *mColumn;
};

class FormatItem : public QListViewItem {
  public:
    FormatItem(KImportColumn *col,int format,QListView *parent)
      : QListViewItem(parent), mColumn(col), mFormat(format)
    {
      setText(0,mColumn->formatName(mFormat));
    }

    KImportColumn *column() { return mColumn; }
    int format() { return mFormat; }

  private:
    KImportColumn *mColumn;
    int mFormat;
};


/**
  This is a generic class for importing line-oriented data from text files. It
  provides a dialog for file selection, preview, separator selection and column
  assignment as well as generic conversion routines. For conversion to special
  data objects, this class has to be inherited by a special class, which
  reimplements the convertRow() function.
*/
KImportDialog::KImportDialog(QWidget* parent)
    : KDialogBase(parent,"importdialog",true,i18n("Import Text File"),Ok|Cancel),
      mSeparator(","),
      mCurrentRow(0)
{
  mData.setAutoDelete( true );

  QVBox *topBox = new QVBox(this);
  setMainWidget(topBox);
  topBox->setSpacing(spacingHint());

  QHBox *fileBox = new QHBox(topBox);
  fileBox->setSpacing(spacingHint());
  new QLabel(i18n("File to import:"),fileBox);
  KURLRequester *urlRequester = new KURLRequester(fileBox);
  connect(urlRequester,SIGNAL(returnPressed(const QString &)),
          SLOT(setFile(const QString &)));
  connect(urlRequester,SIGNAL(urlSelected(const QString &)),
          SLOT(setFile(const QString &)));
  connect(urlRequester->lineEdit(),SIGNAL(textChanged ( const QString & )),
          SLOT(slotUrlChanged(const QString & )));
  mTable = new QTable(5,5,topBox);
  mTable->setMinimumHeight(100);
  connect(mTable,SIGNAL(selectionChanged()),SLOT(tableSelected()));

  QHBox *selectBox = new QHBox(topBox);
  selectBox->setSpacing(spacingHint());

  QButtonGroup *separatorGroup = new QButtonGroup(1,Horizontal,
                                                  i18n("Separator"),selectBox);
  (new QRadioButton(",",separatorGroup))->setChecked(true);
  new QRadioButton(i18n("Tab"),separatorGroup);
  new QRadioButton(i18n("Space"),separatorGroup);
  new QRadioButton("=",separatorGroup);
  new QRadioButton(";",separatorGroup);
  connect(separatorGroup, SIGNAL(clicked(int)),
          this, SLOT(separatorClicked(int)));

  QVBox *assignBox = new QVBox(selectBox);
  assignBox->setSpacing(spacingHint());
  QHBox *listsBox = new QHBox(assignBox);
  listsBox->setSpacing(spacingHint());

  mHeaderList = new QListView(listsBox);
  mHeaderList->addColumn(i18n("Header"));
//  mHeaderList->header()->hide();
  connect(mHeaderList, SIGNAL(selectionChanged(QListViewItem*)),
          this, SLOT(headerSelected(QListViewItem*)));
  connect(mHeaderList,SIGNAL(doubleClicked(QListViewItem*)),
          SLOT(assignColumn(QListViewItem *)));

  mFormatList = new QListView(listsBox);
  mFormatList->addColumn(i18n("Format"));
  connect(mFormatList,SIGNAL(doubleClicked(QListViewItem*)),
          SLOT(assignColumn()));
  connect(mFormatList, SIGNAL(selectionChanged(QListViewItem*)),
          this, SLOT(formatSelected(QListViewItem*)));

  QPushButton *assignButton = new QPushButton(i18n("Assign to Selected Column"),
                                              assignBox);
  connect(assignButton,SIGNAL(clicked()),SLOT(assignColumn()));

  QPushButton *removeButton = new QPushButton(i18n("Remove Assignment From Selected Column"),
                                              assignBox);
  connect(removeButton,SIGNAL(clicked()),SLOT(removeColumn()));

  resize(500,300);

  connect(this,SIGNAL(okClicked()),SLOT(applyConverter()));
  connect(this,SIGNAL(applyClicked()),SLOT(applyConverter()));
  enableButtonOK(!urlRequester->lineEdit()->text().isEmpty());
}

void KImportDialog::slotUrlChanged(const QString & text)
{
    enableButtonOK(!text.isEmpty());
}

bool KImportDialog::setFile(const QString& file)
{
    enableButtonOK(!file.isEmpty());
  kdDebug(5300) << "KImportDialog::setFile(): " << file << endl;

  QFile f(file);

  if (f.open(IO_ReadOnly)) {
    mFile = "";
    QTextStream t(&f);
    mFile = t.read();
//    while (!t.eof()) mFile.append(t.readLine());
    f.close();

    readFile();

    return true;
  } else {
    kdDebug(5300) << " Open failed" << endl;
    return false;
  }
}

void KImportDialog::registerColumns()
{
  QPtrListIterator<KImportColumn> colIt(mColumns);
  for (; colIt.current(); ++colIt) {
    new ColumnItem(*colIt,mHeaderList);
  }
  mHeaderList->setSelected(mHeaderList->firstChild(),true);
}

void KImportDialog::fillTable()
{
//  kdDebug(5300) << "KImportDialog::fillTable()" << endl;

  int row, column;

  for (row = 0; row < mTable->numRows(); ++row)
      for (column = 0; column < mTable->numCols(); ++column)
          mTable->clearCell(row, column);

  for ( row = 0; row < int(mData.count()); ++row ) {
    QValueVector<QString> *rowVector = mData[ row ];
    for( column = 0; column < int(rowVector->size()); ++column ) {
      setCellText( row, column, rowVector->at( column ) );
    }
  }
}

void KImportDialog::readFile( int rows )
{
  kdDebug(5300) << "KImportDialog::readFile(): " << rows << endl;

  mData.clear();

  int row, column;
  enum { S_START, S_QUOTED_FIELD, S_MAYBE_END_OF_QUOTED_FIELD, S_END_OF_QUOTED_FIELD,
         S_MAYBE_NORMAL_FIELD, S_NORMAL_FIELD } state = S_START;

  QChar m_textquote = '"';
  int m_startline = 0;

  QChar x;
  QString field = "";

  row = column = 0;
  QTextStream inputStream(mFile, IO_ReadOnly);
  inputStream.setEncoding(QTextStream::Locale);

  KProgressDialog pDialog(this, 0, i18n("Loading Progress"),
                    i18n("Please wait while the file is loaded."), true);
  pDialog.setAllowCancel(true);
  pDialog.showCancelButton(true);
  pDialog.setAutoClose(true);
  
  KProgress *progress = pDialog.progressBar();
  progress->setRange(0, mFile.contains(mSeparator, false));
  progress->setValue(0);
  int progressValue = 0;
  
  if (progress->maxValue() > 0)  // We have data
    pDialog.show();
    
  while (!inputStream.atEnd() && !pDialog.wasCancelled()) {
    inputStream >> x; // read one char

    // update the dialog if needed
    if (x == mSeparator)
    {
      progress->setValue(progressValue++);
      if (progressValue % 15 == 0) // try not to constantly repaint
        kapp->processEvents();
    }
    
    if (x == '\r') inputStream >> x; // eat '\r', to handle DOS/LOSEDOWS files correctly

    switch (state) {
      case S_START :
        if (x == m_textquote) {
          field += x;
          state = S_QUOTED_FIELD;
        } else if (x == mSeparator) {
          ++column;
        } else if (x == '\n')  {
          ++row;
          column = 0;
        } else {
          field += x;
          state = S_MAYBE_NORMAL_FIELD;
        }
        break;
      case S_QUOTED_FIELD :
        if (x == m_textquote) {
          field += x;
          state = S_MAYBE_END_OF_QUOTED_FIELD;
        } else if (x == '\n') {
          setData(row - m_startline, column, field);
          field = "";
          if (x == '\n') {
            ++row;
            column = 0;
          } else {
            ++column;
          }
          state = S_START;
        } else {
          field += x;
        }
        break;
      case S_MAYBE_END_OF_QUOTED_FIELD :
        if (x == m_textquote) {
          field += x;
          state = S_QUOTED_FIELD;
        } else if (x == mSeparator || x == '\n') {
          setData(row - m_startline, column, field);
          field = "";
          if (x == '\n') {
            ++row;
            column = 0;
          } else {
            ++column;
          }
          state = S_START;
        } else {
          state = S_END_OF_QUOTED_FIELD;
        }
        break;
      case S_END_OF_QUOTED_FIELD :
        if (x == mSeparator || x == '\n') {
          setData(row - m_startline, column, field);
          field = "";
          if (x == '\n') {
            ++row;
            column = 0;
          } else {
            ++column;
          }
          state = S_START;
        } else {
          state = S_END_OF_QUOTED_FIELD;
        }
        break;
      case S_MAYBE_NORMAL_FIELD :
        if (x == m_textquote) {
          field = "";
          state = S_QUOTED_FIELD;
        }
      case S_NORMAL_FIELD :
        if (x == mSeparator || x == '\n') {
          setData(row - m_startline, column, field);
          field = "";
          if (x == '\n') {
            ++row;
            column = 0;
          } else {
            ++column;
          }
          state = S_START;
        } else {
          field += x;
        }
    }

    if ( rows > 0 && row > rows ) break;
  }

  fillTable();
}

void KImportDialog::setCellText(int row, int col, const QString& text)
{
  if (row < 0) return;

  if ((mTable->numRows() - 1) < row) mTable->setNumRows(row + 1);
  if ((mTable->numCols() - 1) < col) mTable->setNumCols(col + 1);

  KImportColumn *c = mColumnDict.find(col);
  QString formattedText;
  if (c) formattedText = c->preview(text,findFormat(col));
  else formattedText = text;
  mTable->setText(row, col, formattedText);
}

void KImportDialog::formatSelected(QListViewItem*)
{
//    kdDebug(5300) << "KImportDialog::formatSelected()" << endl;
}

void KImportDialog::headerSelected(QListViewItem* item)
{
  KImportColumn *col = ((ColumnItem *)item)->column();
  if (!col) return;

  mFormatList->clear();

  QValueList<int> formats = col->formats();

  QValueList<int>::ConstIterator it = formats.begin();
  QValueList<int>::ConstIterator end = formats.end();
  while(it != end) {
    new FormatItem(col,*it,mFormatList);
    ++it;
  }

  QTableSelection selection = mTable->selection(mTable->currentSelection());

  updateFormatSelection(selection.leftCol());
}

void KImportDialog::updateFormatSelection(int column)
{
  FormatItem *item = (FormatItem *)mFormatList->firstChild();

  int format = findFormat(column);
  if (format == KImportColumn::FormatUndefined) format = item->column()->defaultFormat();
  while(item) {
    if (item->format() == format) break;
    item = (FormatItem *)item->nextSibling();
  }
  if (item) mFormatList->setSelected(item,true);
  else mFormatList->setSelected(mFormatList->firstChild(),true);
}

void KImportDialog::tableSelected()
{
  QTableSelection selection = mTable->selection(mTable->currentSelection());

  QListViewItem *item = mHeaderList->firstChild();
  KImportColumn *col = mColumnDict.find(selection.leftCol());
  if (col) {
    while(item) {
      if (item->text(0) == col->header()) {
        break;
      }
      item = item->nextSibling();
    }
  }
  if (item) {
    mHeaderList->setSelected(item,true);
  }

  updateFormatSelection(selection.leftCol());
}

void KImportDialog::separatorClicked(int id)
{
  switch(id) {
    case 0:
      mSeparator = ',';
      break;
    case 1:
      mSeparator = '\t';
      break;
    case 2:
      mSeparator = ' ';
      break;
    case 3:
      mSeparator = '=';
      break;
    case 4:
      mSeparator = ';';
      break;
    default:
      mSeparator = ',';
      break;
  }

  readFile();
}

void KImportDialog::assignColumn(QListViewItem *item)
{
  if (!item) return;

//  kdDebug(5300) << "KImportDialog::assignColumn(): current Col: " << mTable->currentColumn()
//            << endl;

  ColumnItem *colItem = (ColumnItem *)item;

  QTableSelection selection = mTable->selection(mTable->currentSelection());

//  kdDebug(5300) << " l: " << selection.leftCol() << "  r: " << selection.rightCol() << endl;

  for(int i=selection.leftCol();i<=selection.rightCol();++i) {
    if (i >= 0) {
      mTable->horizontalHeader()->setLabel(i,colItem->text(0));
      mColumnDict.replace(i,colItem->column());
      int format = ((FormatItem *)mFormatList->selectedItem())->format();
      mFormats.replace(i,format);
      colItem->column()->addColId(i);
    }
  }

  readFile();
}

void KImportDialog::assignColumn()
{
  assignColumn(mHeaderList->currentItem());
}

void KImportDialog::removeColumn()
{
  QTableSelection selection = mTable->selection(mTable->currentSelection());

//  kdDebug(5300) << " l: " << selection.leftCol() << "  r: " << selection.rightCol() << endl;

  for(int i=selection.leftCol();i<=selection.rightCol();++i) {
    if (i >= 0) {
      mTable->horizontalHeader()->setLabel(i,QString::number(i+1));
      KImportColumn *col = mColumnDict.find(i);
      if (col) {
        mColumnDict.remove(i);
        mFormats.remove(i);
        col->removeColId(i);
      }
    }
  }

  readFile();
}

void KImportDialog::applyConverter()
{
  kdDebug(5300) << "KImportDialog::applyConverter" << endl;

  KProgressDialog pDialog(this, 0, i18n("Importing Progress"),
                    i18n("Please wait while the data is imported."), true);
  pDialog.setAllowCancel(true);
  pDialog.showCancelButton(true);
  pDialog.setAutoClose(true);
  
  KProgress *progress = pDialog.progressBar();
  progress->setRange(0, mTable->numRows()-1);
  progress->setValue(0);

  readFile( 0 );
  
  pDialog.show();
  for( uint i = 1; i < mData.count() && !pDialog.wasCancelled(); ++i ) {
    // i starts with 1 to skip header line
    mCurrentRow = i;
    progress->setValue(i);
    if (i % 5 == 0)  // try to avoid constantly processing events
      kapp->processEvents();
      
    convertRow();
  }
}

int KImportDialog::findFormat(int column)
{
  QMap<int,int>::ConstIterator formatIt = mFormats.find(column);
  int format;
  if (formatIt == mFormats.end()) format = KImportColumn::FormatUndefined;
  else format = *formatIt;

//  kdDebug(5300) << "KImportDialog::findformat(): " << column << ": " << format << endl;

  return format;
}

QString KImportDialog::cell(uint col)
{
  if ( col >= mData[ mCurrentRow ]->size() ) return "";
  else return data( mCurrentRow, col );
}

void KImportDialog::addColumn(KImportColumn *col)
{
  mColumns.append(col);
}

void KImportDialog::setData( uint row, uint col, const QString &value )
{
  QString val;
  for ( uint i = 0; i < value.length(); ++i ) {
    if ( value[i] == '\\' && value[i+1] == 'n' )
      val.append( "\n" );
    else
      val.append( value[i] );
  }

  if ( row >= mData.count() ) {
    mData.resize( row + 1 );
  }
  
  QValueVector<QString> *rowVector = mData[ row ];
  if ( !rowVector ) {
    rowVector = new QValueVector<QString>;
    mData.insert( row, rowVector );
  }
  if ( col >= rowVector->size() ) {
    rowVector->resize( col + 1 );
  }
  
  KImportColumn *c = mColumnDict.find( col );
  if ( c )
  	rowVector->at( col ) = c->preview( val, findFormat(col) );
  else
    rowVector->at( col ) = val;
}

QString KImportDialog::data( uint row, uint col )
{
  return mData[ row ]->at( col );
}
