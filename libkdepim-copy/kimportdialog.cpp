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

// Generic CSV import. Please do not add application specific code to this
// class. Application specific code should go to a subclass provided by the
// application using this dialog.

#include "kimportdialog.h"
#include <q3buttongroup.h>
#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QLineEdit>
#include <q3listview.h>
#include <qradiobutton.h>
#include <QRegExp>
#include <QApplication>
#include <q3table.h>
#include <qtextstream.h>
#include <kvbox.h>

#include <kdebug.h>
#include <kcombobox.h>
#include <kinputdialog.h>
#include <kcomponentdata.h>
#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kprogressdialog.h>

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

QList<int> KImportColumn::formats()
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
    return '(' + value + ')';
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
  mColIds.removeAll(id);
}

QList<int> KImportColumn::colIdList()
{
  return mColIds;
}

QString KImportColumn::convert()
{
  QList<int>::ConstIterator it = mColIds.begin();
  if (it == mColIds.end()) return "";
  else return mDialog->cell(*it);
}


class ColumnItem : public Q3ListViewItem {
  public:
    ColumnItem(KImportColumn *col,Q3ListView *parent) : Q3ListViewItem(parent), mColumn(col)
    {
      setText(0,mColumn->header());
    }

    KImportColumn *column() { return mColumn; }

  private:
    KImportColumn *mColumn;
};

/**
  This is a generic class for importing line-oriented data from text files. It
  provides a dialog for file selection, preview, separator selection and column
  assignment as well as generic conversion routines. For conversion to special
  data objects, this class has to be inherited by a special class, which
  reimplements the convertRow() function.
*/
KImportDialog::KImportDialog(QWidget* parent)
    : KDialog(parent ), mSeparator(","), mCurrentRow(0)
{
  setCaption( i18n( "Import Text File" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  mData.setAutoDelete( true );

  KVBox *topBox = new KVBox(this);
  setMainWidget(topBox);
  topBox->setSpacing(spacingHint());

  KHBox *fileBox = new KHBox(topBox);
  fileBox->setSpacing(spacingHint());
  new QLabel(i18n("File to import:"),fileBox);
  KUrlRequester *urlRequester = new KUrlRequester(fileBox);
  urlRequester->setFilter( "*.csv" );
  connect(urlRequester,SIGNAL(returnPressed(const QString &)),
          SLOT(setFile(const QString &)));
  connect(urlRequester,SIGNAL(urlSelected(const KUrl &)),
          SLOT(setFile(const QString &)));
  connect(urlRequester->lineEdit(),SIGNAL(textChanged ( const QString & )),
          SLOT(slotUrlChanged(const QString & )));
  mTable = new Q3Table(5,5,topBox);
  mTable->setMinimumHeight( 150 );
  connect(mTable,SIGNAL(selectionChanged()),SLOT(tableSelected()));

  KHBox *separatorBox = new KHBox( topBox );
  separatorBox->setSpacing( spacingHint() );

  new QLabel( i18n( "Separator:" ), separatorBox );

  mSeparatorCombo = new KComboBox( separatorBox );
  mSeparatorCombo->addItem( "," );
  mSeparatorCombo->addItem( i18n( "Tab" ) );
  mSeparatorCombo->addItem( i18n( "Space" ) );
  mSeparatorCombo->addItem( "=" );
  mSeparatorCombo->addItem( ";" );
  connect(mSeparatorCombo, SIGNAL( activated(int) ),
          this, SLOT( separatorClicked(int) ) );
  mSeparatorCombo->setCurrentIndex( 0 );

  KHBox *rowsBox = new KHBox( topBox );
  rowsBox->setSpacing( spacingHint() );

  new QLabel( i18n( "Import starts at row:" ), rowsBox );
  mStartRow = new QSpinBox( rowsBox );
  mStartRow->setMinimum( 1 );
/*
  new QLabel( i18n( "And ends at row:" ), rowsBox );
  mEndRow = new QSpinBox( rowsBox );
  mEndRow->setMinValue( 1 );
*/
  KVBox *assignBox = new KVBox(topBox);
  assignBox->setSpacing(spacingHint());

  KHBox *listsBox = new KHBox(assignBox);
  listsBox->setSpacing(spacingHint());

  mHeaderList = new Q3ListView(listsBox);
  mHeaderList->addColumn(i18n("Header"));
  connect(mHeaderList, SIGNAL(selectionChanged(Q3ListViewItem*)),
          this, SLOT(headerSelected(Q3ListViewItem*)));
  connect(mHeaderList,SIGNAL(doubleClicked(Q3ListViewItem*)),
          SLOT(assignColumn(Q3ListViewItem *)));

  mFormatCombo = new KComboBox( listsBox );
  mFormatCombo->setDuplicatesEnabled( false );

  QPushButton *assignButton = new QPushButton(i18n("Assign to Selected Column"),
                                              assignBox);
  connect(assignButton,SIGNAL(clicked()),SLOT(assignColumn()));

  QPushButton *removeButton = new QPushButton(i18n("Remove Assignment From Selected Column"),
                                              assignBox);
  connect(removeButton,SIGNAL(clicked()),SLOT(removeColumn()));

  QPushButton *assignTemplateButton = new QPushButton(i18n("Assign with Template..."),
                                              assignBox);
  connect(assignTemplateButton,SIGNAL(clicked()),SLOT(assignTemplate()));

  QPushButton *saveTemplateButton = new QPushButton(i18n("Save Current Template"),
                                              assignBox);
  connect(saveTemplateButton,SIGNAL(clicked()),SLOT(saveTemplate()));

  resize(500,300);

  connect(this,SIGNAL(okClicked()),SLOT(applyConverter()));
  connect(this,SIGNAL(applyClicked()),SLOT(applyConverter()));
  enableButton(KDialog::Ok, !urlRequester->lineEdit()->text().isEmpty());
}

void KImportDialog::slotUrlChanged(const QString & text)
{
    enableButton(KDialog::Ok,!text.isEmpty());
}

bool KImportDialog::setFile(const KUrl& file)
{
   return setFile(file.path());
}

bool KImportDialog::setFile(const QString& file)
{
    enableButton(KDialog::Ok,!file.isEmpty());
  kDebug(5300) << "KImportDialog::setFile(): " << file << endl;

  QFile f(file);

  if (f.open(QIODevice::ReadOnly)) {
    mFile = "";
    QTextStream t(&f);
    mFile = t.readAll();
//    while (!t.atEnd()) mFile.append(t.readLine());
    f.close();

    readFile();

//    mEndRow->setValue( mData.count() );

    return true;
  } else {
    kDebug(5300) << " Open failed" << endl;
    return false;
  }
}

void KImportDialog::registerColumns()
{
  Q3PtrListIterator<KImportColumn> colIt(mColumns);
  for (; colIt.current(); ++colIt) {
    new ColumnItem(*colIt,mHeaderList);
  }
  mHeaderList->setSelected(mHeaderList->firstChild(),true);
}

void KImportDialog::fillTable()
{
//  kDebug(5300) << "KImportDialog::fillTable()" << endl;

  int row, column;

  for (row = 0; row < mTable->numRows(); ++row)
      for (column = 0; column < mTable->numCols(); ++column)
          mTable->clearCell(row, column);

  for ( row = 0; row < int(mData.count()); ++row ) {
    Q3ValueVector<QString> *rowVector = mData[ row ];
    for( column = 0; column < int(rowVector->size()); ++column ) {
      setCellText( row, column, rowVector->at( column ) );
    }
  }
}

void KImportDialog::readFile( int rows )
{
  kDebug(5300) << "KImportDialog::readFile(): " << rows << endl;

  mData.clear();

  int row, column;
  enum { S_START, S_QUOTED_FIELD, S_MAYBE_END_OF_QUOTED_FIELD, S_END_OF_QUOTED_FIELD,
         S_MAYBE_NORMAL_FIELD, S_NORMAL_FIELD } state = S_START;

  QChar m_textquote = '"';
  int m_startline = 0;

  QChar x;
  QString field = "";

  row = column = 0;
  QTextStream inputStream(&mFile, QIODevice::ReadOnly);
  inputStream.setCodec("ISO 8859-1");

  KProgressDialog pDialog(this, i18n("Loading Progress"),
                    i18n("Please wait while the file is loaded."));
  pDialog.setWindowModality(Qt::WindowModal);
  pDialog.setAllowCancel(true);
  pDialog.showCancelButton(true);
  pDialog.setAutoClose(true);

  QProgressBar *progress = pDialog.progressBar();
  progress->setMaximum( mFile.count(mSeparator, Qt::CaseSensitive) );
  progress->setValue(0);
  int progressValue = 0;

  if (progress->maximum() > 0)  // We have data
    pDialog.show();

  while (!inputStream.atEnd() && !pDialog.wasCancelled()) {
    inputStream >> x; // read one char

    // update the dialog if needed
    if (QString( x ) == mSeparator) // PORT: HUH???
    {
      progress->setValue(progressValue++);
      if (progressValue % 15 == 0) // try not to constantly repaint
        qApp->processEvents();
    }

    if (x == '\r') inputStream >> x; // eat '\r', to handle DOS/LOSEDOWS files correctly

    switch (state) {
      case S_START :
        if (x == m_textquote) {
          field += x;
          state = S_QUOTED_FIELD;
        } else if (QString( x ) == mSeparator) {
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
        } else if (QString( x ) == mSeparator || x == '\n') {
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
        if (QString( x ) == mSeparator || x == '\n') {
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
        if (QString( x ) == mSeparator || x == '\n') {
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

void KImportDialog::formatSelected(Q3ListViewItem*)
{
//    kDebug(5300) << "KImportDialog::formatSelected()" << endl;
}

void KImportDialog::headerSelected(Q3ListViewItem* item)
{
  KImportColumn *col = ((ColumnItem *)item)->column();

  if (!col) return;

  mFormatCombo->clear();

  QList<int> formats = col->formats();

  QList<int>::ConstIterator it = formats.begin();
  QList<int>::ConstIterator end = formats.end();
  while(it != end) {
    mFormatCombo->insertItem( *it - 1, col->formatName(*it) );
    ++it;
  }

  Q3TableSelection selection = mTable->selection(mTable->currentSelection());

  updateFormatSelection(selection.leftCol());
}

void KImportDialog::updateFormatSelection(int column)
{
  int format = findFormat(column);

  if ( format == KImportColumn::FormatUndefined )
    mFormatCombo->setCurrentIndex( 0 );
  else
    mFormatCombo->setCurrentIndex( format - 1 );
}

void KImportDialog::tableSelected()
{
  Q3TableSelection selection = mTable->selection(mTable->currentSelection());

  Q3ListViewItem *item = mHeaderList->firstChild();
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

void KImportDialog::assignColumn(Q3ListViewItem *item)
{
  if (!item) return;

//  kDebug(5300) << "KImportDialog::assignColumn(): current Col: " << mTable->currentColumn()
//            << endl;

  ColumnItem *colItem = (ColumnItem *)item;

  Q3TableSelection selection = mTable->selection(mTable->currentSelection());

//  kDebug(5300) << " l: " << selection.leftCol() << "  r: " << selection.rightCol() << endl;

  for(int i=selection.leftCol();i<=selection.rightCol();++i) {
    if (i >= 0) {
      mTable->horizontalHeader()->setLabel(i,colItem->text(0));
      mColumnDict.replace(i,colItem->column());
      int format = mFormatCombo->currentIndex() + 1;
      mFormats.insert(i,format);
      colItem->column()->addColId(i);
    }
  }

  readFile();
}

void KImportDialog::assignColumn()
{
  assignColumn(mHeaderList->currentItem());
}

void KImportDialog::assignTemplate()
{
  QMap<int,int> columnMap;
  QMap<QString, QString> fileMap;
  QStringList templates;

  // load all template files
  const QStringList list = KGlobal::dirs()->findAllResources( "data" , KGlobal::mainComponent().componentName() +
                                                              "/csv-templates/*.desktop",
                                                              KStandardDirs::Recursive |
                                                              KStandardDirs::NoDuplicates );

  for ( QStringList::const_iterator it = list.begin(); it != list.end(); ++it )
  {
    KConfig config( *it, KConfig::OnlyLocal);

    if ( !config.hasGroup( "csv column map" ) )
      continue;

    KConfigGroup group( &config, "Misc" );
    templates.append( group.readEntry( "Name" ) );
    fileMap.insert( group.readEntry( "Name" ), *it );
  }

  // let the user chose, what to take
  bool ok = false;
  QString tmp;
  tmp = KInputDialog::getItem( i18n( "Template Selection" ),
                  i18n( "Please select a template, that matches the CSV file:" ),
                  templates, 0, false, &ok, this );

  if ( !ok )
    return;

  KConfig _config( fileMap[ tmp ], KConfig::OnlyLocal );
  KConfigGroup config(&_config, "General" );
  int numColumns = config.readEntry( "Columns", 0 );
  int format = config.readEntry( "Format", 0 );

  // create the column map
  config.changeGroup( "csv column map" );
  for ( int i = 0; i < numColumns; ++i ) {
    int col = config.readEntry( QString::number( i ), 0 );
    columnMap.insert( i, col );
  }

  // apply the column map
  for ( int i = 0; i < columnMap.count(); ++i ) {
    int tableColumn = columnMap[i];
    if ( tableColumn == -1 )
      continue;
    KImportColumn *col = mColumns.at(i);
    mTable->horizontalHeader()->setLabel( tableColumn, col->header() );
    mColumnDict.replace( tableColumn, col );
    mFormats.insert( tableColumn, format );
    col->addColId( tableColumn );
  }

  readFile();
}

void KImportDialog::removeColumn()
{
  Q3TableSelection selection = mTable->selection(mTable->currentSelection());

//  kDebug(5300) << " l: " << selection.leftCol() << "  r: " << selection.rightCol() << endl;

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
  kDebug(5300) << "KImportDialog::applyConverter" << endl;

  KProgressDialog pDialog(this, i18n("Importing Progress"),
                    i18n("Please wait while the data is imported."));
  pDialog.setWindowModality(Qt::WindowModal);
  pDialog.setAllowCancel(true);
  pDialog.showCancelButton(true);
  pDialog.setAutoClose(true);

  QProgressBar *progress = pDialog.progressBar();
  progress->setMaximum( mTable->numRows()-1 );
  progress->setValue(0);

  readFile( 0 );

  pDialog.show();
  for( int i = mStartRow->value() - 1; i < int(mData.count()) && !pDialog.wasCancelled(); ++i ) {
    mCurrentRow = i;
    progress->setValue(i);
    if (i % 5 == 0)  // try to avoid constantly processing events
      qApp->processEvents();

    convertRow();
  }
}

int KImportDialog::findFormat(int column)
{
  QMap<int,int>::ConstIterator formatIt = mFormats.find(column);
  int format;
  if (formatIt == mFormats.end()) format = KImportColumn::FormatUndefined;
  else format = *formatIt;

//  kDebug(5300) << "KImportDialog::findformat(): " << column << ": " << format << endl;

  return format;
}

QString KImportDialog::cell(int col)
{
  if ( col >= mData[ mCurrentRow ]->size() ) return "";
  else return data( mCurrentRow, col );
}

void KImportDialog::addColumn(KImportColumn *col)
{
  mColumns.append(col);
}

void KImportDialog::setData( int row, int col, const QString &value )
{
  QString val = value;
  val.replace( "\\n", "\n" );

  if ( row >= int(mData.count()) ) {
    mData.resize( row + 1 );
  }

  Q3ValueVector<QString> *rowVector = mData[ row ];
  if ( !rowVector ) {
    rowVector = new Q3ValueVector<QString>;
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

QString KImportDialog::data( int row, int col )
{
  return mData[ row ]->at( col );
}

void KImportDialog::saveTemplate()
{
  QString fileName = KFileDialog::getSaveFileName(
                      KStandardDirs::locateLocal( "data", KGlobal::mainComponent().componentName() + "/csv-templates/" ),
                      "*.desktop", this );

  if ( fileName.isEmpty() )
    return;

  if ( !fileName.contains( ".desktop" ) )
    fileName += ".desktop";

  QString name = KInputDialog::getText( i18n( "Template Name" ), i18n( "Please enter a name for the template:" ) );

  if ( name.isEmpty() )
    return;

  KConfig _config( fileName  );
  KConfigGroup config(&_config, "General" );
  config.writeEntry( "Columns", mColumns.count() );
  config.writeEntry( "Format", mFormatCombo->currentIndex() + 1 );

  config.changeGroup( "Misc" );
  config.writeEntry( "Name", name );

  config.changeGroup( "csv column map" );

  KImportColumn *column;
  int counter = 0;
  for ( column = mColumns.first(); column; column = mColumns.next() ) {
    QList<int> list = column->colIdList();
    if ( list.count() > 0 )
      config.writeEntry( QString::number( counter ), list[ 0 ] );
    else
      config.writeEntry( QString::number( counter ), -1 );
    counter++;
  }

  config.sync();
}
