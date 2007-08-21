/*
    kscoring.cpp

    Copyright (c) 2001 Mathias Waack
    Copyright (C) 2005 by Volker Krause <volker.krause@rwth-aachen.de>

    Author: Mathias Waack <mathias@atoll-net.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/
#ifdef KDE_USE_FINAL
#undef QT_NO_ASCII_CAST
#endif

#undef QT_NO_COMPAT

#include <iostream>

#include <QFile>
#include <qdom.h>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QTextEdit>
//Added by qt3to4:
#include <QTextStream>
#include <Q3PtrList>
#include <QVBoxLayout>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kinputdialog.h>

#include "kscoring.h"
#include "kscoringeditor.h"


//----------------------------------------------------------------------------
// a small function to encode attribute values, code stolen from QDom
static QString toXml(const QString& str)
{
  QString tmp(str);
  uint len = tmp.length();
  uint i = 0;
  while ( i < len ) {
    if (tmp[(int)i] == '<') {
      tmp.replace(i, 1, "&lt;");
      len += 3;
      i += 4;
    } else if (tmp[(int)i] == '"') {
      tmp.replace(i, 1, "&quot;");
      len += 5;
      i += 6;
    } else if (tmp[(int)i] == '&') {
       tmp.replace(i, 1, "&amp;");
       len += 4;
       i += 5;
    } else if (tmp[(int)i] == '>') {
       tmp.replace(i, 1, "&gt;");
       len += 3;
       i += 4;
    } else {
       ++i;
    }
  }

  return tmp;
}


// small dialog to display the messages from NotifyAction
NotifyDialog* NotifyDialog::me = 0;
NotifyDialog::NotesMap NotifyDialog::dict;

NotifyDialog::NotifyDialog(QWidget* parent)
  : KDialog( parent )
{
  setCaption( i18n( "Notify Message" ) );
  setButtons( Close );
  setDefaultButton( Close );
  setModal( true );

  QFrame *f = new QFrame( this );
  setMainWidget ( f );
  QVBoxLayout *topL = new QVBoxLayout(f);
  note = new QLabel(f);
  note->setTextFormat(Qt::RichText);
  topL->addWidget(note);
  QCheckBox *check = new QCheckBox(i18n("Do not show this message again"),f);
  check->setChecked(true);
  topL->addWidget(check);
  connect(check,SIGNAL(toggled(bool)),SLOT(slotShowAgainToggled(bool)));
}

void NotifyDialog::slotShowAgainToggled(bool flag)
{
  dict.remove(msg);
  dict.insert(msg,!flag);
  kDebug(5100) <<"note \"" << note <<"\" will popup again:" << flag;
}

void NotifyDialog::display(ScorableArticle& a, const QString& s)
{
  kDebug(5100) <<"displaying message";
  if (!me) me = new NotifyDialog();
  me->msg = s;

  NotesMap::Iterator i = dict.find(s);
  if (i == dict.end() || i.value()) {
    QString msg = i18n("Article\n<b>%1</b><br /><b>%2</b><br />caused the following"
                       " note to appear:<br />%3",
                  a.from(),
                  a.subject(),
                  s);
    me->note->setText(msg);
    if ( i == dict.end() )
    {
      dict.remove(s);
      i = dict.insert(s, false);
    }
    me->adjustSize();
    me->exec();
  }
}


//----------------------------------------------------------------------------
ScorableArticle::~ScorableArticle()
{
}

void ScorableArticle::displayMessage(const QString& note)
{
  NotifyDialog::display(*this,note);
}

//----------------------------------------------------------------------------
ScorableGroup::~ScorableGroup()
{
}

// the base class for all actions
ActionBase::ActionBase()
{
  kDebug(5100) <<"new Action" << this;
}

ActionBase::~ActionBase()
{
  kDebug(5100) <<"delete Action" << this;
}


QStringList ActionBase::userNames()
{
  QStringList l;
  l << userName(SETSCORE);
  l << userName(NOTIFY);
  l << userName(COLOR);
  l << userName(MARKASREAD);
  return l;
}

ActionBase* ActionBase::factory(int type, const QString &value)
{
  switch (type) {
    case SETSCORE: return new ActionSetScore(value);
    case NOTIFY:   return new ActionNotify(value);
    case COLOR:    return new ActionColor(value);
    case MARKASREAD: return new ActionMarkAsRead();
  default:
    kWarning(5100) <<"unknown type" << type <<" in ActionBase::factory()";
    return 0;
  }
}

QString ActionBase::userName(int type)
{
  switch (type) {
    case SETSCORE: return i18n("Adjust Score");
    case NOTIFY:   return i18n("Display Message");
    case COLOR:    return i18n("Colorize Header");
    case MARKASREAD: return i18n("Mark As Read");
  default:
    kWarning(5100) <<"unknown type" << type <<" in ActionBase::userName()";
    return 0;
  }
}

int ActionBase::getTypeForName(const QString& name)
{
  if (name == "SETSCORE") return SETSCORE;
  else if (name == "NOTIFY") return NOTIFY;
  else if (name == "COLOR") return COLOR;
  else if (name == "MARKASREAD") return MARKASREAD;
  else {
    kWarning(5100) <<"unknown type string" << name
                    << "in ActionBase::getTypeForName()";
    return -1;
  }
}

int ActionBase::getTypeForUserName(const QString& name)
{
  if (name == userName(SETSCORE)) return SETSCORE;
  else if (name == userName(NOTIFY)) return NOTIFY;
  else if (name == userName(COLOR)) return COLOR;
  else if ( name == userName(MARKASREAD) ) return MARKASREAD;
  else {
    kWarning(5100) <<"unknown type string" << name
                    << "in ActionBase::getTypeForUserName()";
    return -1;
  }
}

// the set score action
ActionSetScore::ActionSetScore(short v)
  : val(v)
{
}

ActionSetScore::ActionSetScore(const QString& s)
{
  val = s.toShort();
}

ActionSetScore::ActionSetScore(const ActionSetScore& as)
  : ActionBase(),
    val(as.val)
{
}

ActionSetScore::~ActionSetScore()
{
}

QString ActionSetScore::toString() const
{
  QString a;
  a += "<Action type=\"SETSCORE\" value=\"" + QString::number(val) + "\" />";
  return a;
}

void ActionSetScore::apply(ScorableArticle& a) const
{
  a.addScore(val);
}

ActionSetScore* ActionSetScore::clone() const
{
  return new ActionSetScore(*this);
}

// the color action
ActionColor::ActionColor(const QColor& c)
  : ActionBase(), color(c)
{
}

ActionColor::ActionColor(const QString& s)
  : ActionBase()
{
  setValue(s);
}

ActionColor::ActionColor(const ActionColor& a)
  : ActionBase(), color(a.color)
{
}

ActionColor::~ActionColor()
{}

QString ActionColor::toString() const
{
  QString a;
  a += "<Action type=\"COLOR\" value=\"" + toXml(color.name()) + "\" />";
  return a;
}

void ActionColor::apply(ScorableArticle& a) const
{
  a.changeColor(color);
}

ActionColor* ActionColor::clone() const
{
  return new ActionColor(*this);
}


// the notify action
ActionNotify::ActionNotify(const QString& s)
{
  note = s;
}

ActionNotify::ActionNotify(const ActionNotify& an)
  : ActionBase()
{
  note = an.note;
}

QString ActionNotify::toString() const
{
  return "<Action type=\"NOTIFY\" value=\"" + toXml(note) + "\" />";
}

void ActionNotify::apply(ScorableArticle& a) const
{
  a.displayMessage(note);
}

ActionNotify* ActionNotify::clone() const
{
  return new ActionNotify(*this);
}


// mark as read action
ActionMarkAsRead::ActionMarkAsRead() :
  ActionBase()
{
}

ActionMarkAsRead::ActionMarkAsRead( const ActionMarkAsRead &action ) :
  ActionBase()
{
  Q_UNUSED( action );
}

QString ActionMarkAsRead::toString() const
{
  return "<Action type=\"MARKASREAD\"/>";
}

void ActionMarkAsRead::apply( ScorableArticle &article ) const
{
  article.markAsRead();
}

ActionMarkAsRead* ActionMarkAsRead::clone() const
{
  return new ActionMarkAsRead(*this);
}

//----------------------------------------------------------------------------
NotifyCollection::NotifyCollection()
{
  notifyList.setAutoDelete(true);
}

NotifyCollection::~NotifyCollection()
{
}

void NotifyCollection::addNote(const ScorableArticle& a, const QString& note)
{
  article_list *l = notifyList.find(note);
  if (!l) {
    notifyList.insert(note,new article_list);
    l = notifyList.find(note);
  }
  article_info i;
  i.from = a.from();
  i.subject = a.subject();
  l->append(i);
}

QString NotifyCollection::collection() const
{
  QString notifyCollection = i18n("<h1>List of collected notes</h1>");
  notifyCollection += "<p><ul>";
  // first look thru the notes and create one string
  Q3DictIterator<article_list> it(notifyList);
  for(;it.current();++it) {
    const QString& note = it.currentKey();
    notifyCollection += "<li>" + note + "<ul>";
    article_list* alist = it.current();
    article_list::Iterator ait;
    for(ait = alist->begin(); ait != alist->end(); ++ait) {
      notifyCollection += "<li><b>From: </b>" + (*ait).from + "<br>";
      notifyCollection += "<b>Subject: </b>" + (*ait).subject;
    }
    notifyCollection += "</ul>";
  }
  notifyCollection += "</ul>";

  return notifyCollection;
}

void NotifyCollection::displayCollection(QWidget *p) const
{
  //KMessageBox::information(p,collection(),i18n("Collected Notes"));
  KDialog *dlg = new KDialog( p );
  dlg->setCaption( i18n( "Collected Notes" ) );
  dlg->setButtons( KDialog::Close );
  dlg->setDefaultButton( KDialog::Close );
  dlg->setModal( false );
  QTextEdit *text = new QTextEdit(dlg);
  text->setReadOnly(true);
  text->setText(collection());
  dlg->setMainWidget(text);
  dlg->setMinimumWidth(300);
  dlg->setMinimumHeight(300);
  dlg->show();
}

//----------------------------------------------------------------------------
KScoringExpression::KScoringExpression(const QString& h, const QString& t, const QString& n, const QString& ng)
  : header(h), expr_str(n)
{
  if (t == "MATCH" ) {
    cond = MATCH;
    expr.setPattern(expr_str);
    expr.setCaseSensitivity(Qt::CaseInsensitive);
  }
  else if ( t == "MATCHCS" ) {
    cond = MATCHCS;
    expr.setPattern( expr_str );
    expr.setCaseSensitivity( Qt::CaseSensitive );
  }
  else if (t == "CONTAINS" ) cond = CONTAINS;
  else if (t == "EQUALS" ) cond = EQUALS;
  else if (t == "GREATER") {
    cond = GREATER;
    expr_int = expr_str.toInt();
  }
  else if (t == "SMALLER") {
    cond = SMALLER;
    expr_int = expr_str.toInt();
  }
  else {
    kDebug(5100) <<"unknown match type in new expression";
  }

  neg = ng.toInt();
  c_header = header.toLatin1();

  kDebug(5100) <<"new expr:" << c_header << t 
                << expr_str << neg;
}

// static
int KScoringExpression::getConditionForName(const QString& s)
{
  if (s == getNameForCondition(CONTAINS)) return CONTAINS;
  else if (s == getNameForCondition(MATCH)) return MATCH;
  else if (s == getNameForCondition(MATCHCS)) return MATCHCS;
  else if (s == getNameForCondition(EQUALS)) return EQUALS;
  else if (s == getNameForCondition(SMALLER)) return SMALLER;
  else if (s == getNameForCondition(GREATER)) return GREATER;
  else {
    kWarning(5100) <<"unknown condition name" << s
                    << "in KScoringExpression::getConditionForName()";
    return -1;
  }
}

// static
QString KScoringExpression::getNameForCondition(int cond)
{
  switch (cond) {
  case CONTAINS: return i18n("Contains Substring");
  case MATCH: return i18n("Matches Regular Expression");
  case MATCHCS: return i18n("Matches Regular Expression (Case Sensitive)");
  case EQUALS: return i18n("Is Exactly the Same As");
  case SMALLER: return i18n("Less Than");
  case GREATER: return i18n("Greater Than");
  default:
    kWarning(5100) <<"unknown condition" << cond
                    << "in KScoringExpression::getNameForCondition()";
    return "";
  }
}

// static
QStringList KScoringExpression::conditionNames()
{
  QStringList l;
  l << getNameForCondition(CONTAINS);
  l << getNameForCondition(MATCH);
  l << getNameForCondition(MATCHCS);
  l << getNameForCondition(EQUALS);
  l << getNameForCondition(SMALLER);
  l << getNameForCondition(GREATER);
  return l;
}

// static
QStringList KScoringExpression::headerNames()
{
  QStringList l;
  l.append("From");
  l.append("Message-ID");
  l.append("Subject");
  l.append("Date");
  l.append("References");
  l.append("NNTP-Posting-Host");
  l.append("Bytes");
  l.append("Lines");
  l.append("Xref");
  return l;
}

KScoringExpression::~KScoringExpression()
{
}

bool KScoringExpression::match(ScorableArticle& a) const
{
  //kDebug(5100) <<"matching against header" << c_header;
  bool res = true;
  QString head;

  if (header == "From")
    head = a.from();
  else if (header == "Subject")
    head = a.subject();
  else
    head = a.getHeaderByType(c_header);

  if (!head.isEmpty()) {
    switch (cond) {
    case EQUALS:
      res = (head.toLower() == expr_str.toLower());
      break;
    case CONTAINS:
      res = (head.toLower().indexOf(expr_str.toLower()) >= 0);
      break;
    case MATCH:
    case MATCHCS:
      res = (expr.indexIn(head)!=-1);
      break;
    case GREATER:
      res = (head.toInt() > expr_int);
      break;
    case SMALLER:
      res = (head.toInt() < expr_int);
      break;
    default:
      kDebug(5100) <<"unknown match";
      res = false;
    }
  }
  else res = false;
//  kDebug(5100) <<"matching returns" << res;
  return (neg)?!res:res;
}

void KScoringExpression::write(QTextStream& st) const
{
  st << toString();
}

QString KScoringExpression::toString() const
{
//   kDebug(5100) <<"KScoringExpression::toString() starts";
//   kDebug(5100) <<"header is" << header;
//   kDebug(5100) <<"expr is" << expr_str;
//   kDebug(5100) <<"neg is" << neg;
//   kDebug(5100) <<"type is" << getType();
  QString e;
  e += "<Expression neg=\"" + QString::number(neg?1:0)
    + "\" header=\"" + header
    + "\" type=\"" + getTypeString()
    + "\" expr=\"" + toXml(expr_str)
    + "\" />";
//   kDebug(5100) <<"KScoringExpression::toString() finished";
  return e;
}

QString KScoringExpression::getTypeString() const
{
  return KScoringExpression::getTypeString(cond);
}

QString KScoringExpression::getTypeString(int cond)
{
  switch (cond) {
  case CONTAINS: return "CONTAINS";
  case MATCH: return "MATCH";
  case MATCHCS: return "MATCHCS";
  case EQUALS: return "EQUALS";
  case SMALLER: return "SMALLER";
  case GREATER: return "GREATER";
  default:
    kWarning(5100) <<"unknown cond" << cond <<" in KScoringExpression::getTypeString()";
    return "";
  }
}

int  KScoringExpression::getType() const
{
  return cond;
}

//----------------------------------------------------------------------------
KScoringRule::KScoringRule(const QString& n )
  : name(n), link(AND)
{
  expressions.setAutoDelete(true);
  actions.setAutoDelete(true);
}

KScoringRule::KScoringRule(const KScoringRule& r)
{
  kDebug(5100) <<"copying rule" << r.getName();
  name = r.getName();
  expressions.setAutoDelete(true);
  actions.setAutoDelete(true);
  // copy expressions
  expressions.clear();
  const ScoreExprList& rexpr = r.expressions;
  Q3PtrListIterator<KScoringExpression> it(rexpr);
  for ( ; it.current(); ++it ) {
    KScoringExpression *t = new KScoringExpression(**it);
    expressions.append(t);
  }
  // copy actions
  actions.clear();
  const ActionList& ract = r.actions;
  Q3PtrListIterator<ActionBase> ait(ract);
  for ( ; ait.current(); ++ait ) {
    ActionBase *t = *ait;
    actions.append(t->clone());
  }
  // copy groups, servers, linkmode and expires
  groups = r.groups;
  expires = r.expires;
  link = r.link;
}

KScoringRule::~KScoringRule()
{
  cleanExpressions();
  cleanActions();
}

void KScoringRule::cleanExpressions()
{
  // the expressions is setAutoDelete(true)
  expressions.clear();
}

void KScoringRule::cleanActions()
{
  // the actions is setAutoDelete(true)
  actions.clear();
}

void KScoringRule::addExpression( KScoringExpression* expr)
{
  kDebug(5100) <<"KScoringRule::addExpression";
  expressions.append(expr);
}

void KScoringRule::addAction(int type, const QString& val)
{
  ActionBase *action = ActionBase::factory(type,val);
  addAction(action);
}

void KScoringRule::addAction(ActionBase* a)
{
  kDebug(5100) <<"KScoringRule::addAction()" << a->toString();
  actions.append(a);
}

void KScoringRule::setLinkMode(const QString& l)
{
  if (l == "OR") link = OR;
  else link = AND;
}

void KScoringRule::setExpire(const QString& e)
{
  if (e != "never") {
    QStringList l = e.split('-', QString::SkipEmptyParts);
    Q_ASSERT( l.count() == 3 );
    expires.setYMD( l.at(0).toInt(), l.at(1).toInt(), l.at(2).toInt() );
  }
  kDebug(5100) <<"Rule" << getName() <<" expires at" << getExpireDateString();
}

bool KScoringRule::matchGroup(const QString& group) const
{
  for(GroupList::ConstIterator i=groups.begin(); i!=groups.end();++i) {
    QRegExp e(*i);
    if (e.indexIn(group, 0) != -1 &&
	    e.matchedLength() == group.length())
        return true;
  }
  return false;
}

void KScoringRule::applyAction(ScorableArticle& a) const
{
  Q3PtrListIterator<ActionBase> it(actions);
  for(; it.current(); ++it) {
    it.current()->apply(a);
  }
}

void KScoringRule::applyRule(ScorableArticle& a) const
{
  // kDebug(5100) <<"checking rule" << name;
  // kDebug(5100)  <<" for article from"
  //              << a->from()->asUnicodeString();
  bool oper_and = (link == AND);
  bool res = true;
  Q3PtrListIterator<KScoringExpression> it(expressions);
  //kDebug(5100) <<"checking" << expressions.count() <<" expressions";
  for (; it.current(); ++it) {
    Q_ASSERT( it.current() );
    res = it.current()->match(a);
    if (!res && oper_and) return;
    else if (res && !oper_and) break;
  }
  if (res) applyAction(a);
}

void KScoringRule::applyRule(ScorableArticle& a /*, const QString& s*/, const QString& g) const
{
  // check if one of the groups match
  for (QStringList::ConstIterator i = groups.begin(); i != groups.end(); ++i) {
    if (QRegExp(*i).indexIn(g) != -1) {
      applyRule(a);
      return;
    }
  }
}

void KScoringRule::write(QTextStream& s) const
{
  s << toString();
}

QString KScoringRule::toString() const
{
  //kDebug(5100) <<"KScoringRule::toString() starts";
  QString r;
  r += "<Rule name=\"" + toXml(name) + "\" linkmode=\"" + getLinkModeName();
  r += "\" expires=\"" + getExpireDateString() + "\">";
  //kDebug(5100) <<"building grouplist...";
  for(GroupList::ConstIterator i=groups.begin();i!=groups.end();++i) {
    r += "<Group name=\"" + toXml(*i) + "\" />";
  }
  //kDebug(5100) <<"building expressionlist...";
  Q3PtrListIterator<KScoringExpression> eit(expressions);
  for (; eit.current(); ++eit) {
    r += eit.current()->toString();
  }
  //kDebug(5100) <<"building actionlist...";
  Q3PtrListIterator<ActionBase> ait(actions);
  for (; ait.current(); ++ait) {
    r += ait.current()->toString();
  }
  r += "</Rule>";
  //kDebug(5100) <<"KScoringRule::toString() finished";
  return r;
}

QString KScoringRule::getLinkModeName() const
{
  switch (link) {
  case AND: return "AND";
  case OR: return "OR";
  default: return "AND";
  }
}

QString KScoringRule::getExpireDateString() const
{
  if (expires.isNull()) return "never";
  else {
    return QString::number(expires.year()) + QString('-')
      + QString::number(expires.month()) + QString('-')
      + QString::number(expires.day());
  }
}

bool KScoringRule::isExpired() const
{
  return (expires.isValid() && (expires < QDate::currentDate()));
}



//----------------------------------------------------------------------------
KScoringManager::KScoringManager(const QString& appName)
  :  cacheValid(false)//, _s(0)
{
  allRules.setAutoDelete(true);
  // determine filename of the scorefile
  if(appName.isEmpty())
    mFilename = KGlobal::dirs()->saveLocation("appdata") + "/scorefile";
  else
    mFilename = KGlobal::dirs()->saveLocation("data") + '/' + appName + "/scorefile";
  // open the score file
  load();
}


KScoringManager::~KScoringManager()
{
}

void KScoringManager::load()
{
  QDomDocument sdoc("Scorefile");
  QFile f( mFilename );
  if ( !f.open( QIODevice::ReadOnly ) )
    return;
  if ( !sdoc.setContent( &f ) ) {
    f.close();
    kDebug(5100) <<"loading the scorefile failed";
    return;
  }
  f.close();
  kDebug(5100) <<"loaded the scorefile, creating internal representation";
  allRules.clear();
  createInternalFromXML(sdoc);
  expireRules();
  kDebug(5100) <<"ready, got" << allRules.count() <<" rules";
}

void KScoringManager::save()
{
  kDebug(5100) <<"KScoringManager::save() starts";
  QFile f( mFilename );
  if ( !f.open( QIODevice::WriteOnly ) )
    return;
  QTextStream stream(&f);
  stream.setCodec("UTF-8");
  kDebug(5100) <<"KScoringManager::save() creating xml";
  createXMLfromInternal().save(stream,2);
  kDebug(5100) <<"KScoringManager::save() finished";
}

QDomDocument KScoringManager::createXMLfromInternal()
{
  // I was'nt able to create a QDomDocument in memory:(
  // so I write the content into a string, which is really stupid
  QDomDocument sdoc("Scorefile");
  QString ss; // scorestring
  ss += "<?xml version = '1.0'?><!DOCTYPE Scorefile >";
  ss += toString();
  ss += "</Scorefile>\n";
  kDebug(5100) <<"KScoringManager::createXMLfromInternal():" << endl << ss;
  sdoc.setContent(ss);
  return sdoc;
}

QString KScoringManager::toString() const
{
  QString s;
  s += "<Scorefile>\n";
  Q3PtrListIterator<KScoringRule> it(allRules);
  for( ; it.current(); ++it) {
    s += it.current()->toString();
  }
  return s;
}

void KScoringManager::expireRules()
{
  for ( KScoringRule *cR = allRules.first(); cR; cR=allRules.next()) {
    if (cR->isExpired()) {
      kDebug(5100) <<"Rule" << cR->getName() <<" is expired, deleting it";
      allRules.remove();
    }
  }
}

void KScoringManager::createInternalFromXML(QDomNode n)
{
  static KScoringRule *cR = 0; // the currentRule
  // the XML file was parsed and now we simply traverse the resulting tree
  if ( !n.isNull() ) {
    kDebug(5100) <<"inspecting node of type" << n.nodeType()
                  << "named" << n.toElement().tagName();

    switch (n.nodeType()) {
    case QDomNode::DocumentNode: {
      // the document itself
      break;
    }
    case QDomNode::ElementNode: {
      // Server, Newsgroup, Rule, Expression, Action
      QDomElement e = n.toElement();
      //kDebug(5100) <<"The name of the element is"
      //<< e.tagName().toLatin1();
      QString s = e.tagName();
      if (s == "Rule") {
        cR = new KScoringRule(e.attribute("name"));
        cR->setLinkMode(e.attribute("linkmode"));
        cR->setExpire(e.attribute("expires"));
        addRuleInternal(cR);
      }
      else if (s == "Group") {
        Q_CHECK_PTR(cR);
        cR->addGroup( e.attribute("name") );
      }
      else if (s == "Expression") {
        cR->addExpression(new KScoringExpression(e.attribute("header"),
                                                 e.attribute("type"),
                                                 e.attribute("expr"),
                                                 e.attribute("neg")));
      }
      else if (s == "Action") {
        Q_CHECK_PTR(cR);
        cR->addAction(ActionBase::getTypeForName(e.attribute("type")),
                      e.attribute("value"));
      }
      break;
    }
    default: // kDebug(5100) <<"unknown DomNode::type";
      ;
    }
    QDomNodeList nodelist = n.childNodes();
    int cnt = nodelist.count();
    //kDebug(5100) <<"recursive checking" << cnt <<" nodes";
    for (int i=0;i<cnt;++i)
      createInternalFromXML(nodelist.item(i));
  }
}

KScoringRule* KScoringManager::addRule(const ScorableArticle& a, QString group, short score)
{
  KScoringRule *rule = new KScoringRule(findUniqueName());
  rule->addGroup( group );
  rule->addExpression(
    new KScoringExpression("From","CONTAINS",
                            a.from(),"0"));
  if (score) rule->addAction(new ActionSetScore(score));
  rule->setExpireDate(QDate::currentDate().addDays(30));
  addRule(rule);
  KScoringEditor *edit = KScoringEditor::createEditor(this);
  edit->setRule(rule);
  edit->show();
  setCacheValid(false);
  return rule;
}

KScoringRule* KScoringManager::addRule(KScoringRule* expr)
{
  int i = allRules.findRef(expr);
  if (i == -1) {
    // only add a rule we don't know
    addRuleInternal(expr);
  }
  else {
    emit changedRules();
  }
  return expr;
}

KScoringRule* KScoringManager::addRule()
{
  KScoringRule *rule = new KScoringRule(findUniqueName());
  addRule(rule);
  return rule;
}

void KScoringManager::addRuleInternal(KScoringRule *e)
{
  allRules.append(e);
  setCacheValid(false);
  emit changedRules();
  kDebug(5100) <<"KScoringManager::addRuleInternal" << e->getName();
}

void KScoringManager::cancelNewRule(KScoringRule *r)
{
  // if e was'nt previously added to the list of rules, we delete it
  int i = allRules.findRef(r);
  if (i == -1) {
    kDebug(5100) <<"deleting rule" << r->getName();
    deleteRule(r);
  }
  else {
    kDebug(5100) <<"rule" << r->getName() <<" not deleted";
  }
}

void KScoringManager::setRuleName(KScoringRule *r, const QString& s)
{
  bool cont = true;
  QString text = s;
  QString oldName = r->getName();
  while (cont) {
    cont = false;
    Q3PtrListIterator<KScoringRule> it(allRules);
    for (; it.current(); ++it) {
      if ( it.current() != r && it.current()->getName() == text ) {
        kDebug(5100) <<"rule name" << text <<" is not unique";
	text = KInputDialog::getText(i18n("Choose Another Rule Name"),
			i18n("The rule name is already assigned, please choose another name:"),
			text);
        cont = true;
        break;
      }
    }
  }
  if (text != oldName) {
    r->setName(text);
    emit changedRuleName(oldName,text);
  }
}

void KScoringManager::deleteRule(KScoringRule *r)
{
  int i = allRules.findRef(r);
  if (i != -1) {
    allRules.remove();
    emit changedRules();
  }
}

void KScoringManager::editRule(KScoringRule *e, QWidget *w)
{
  KScoringEditor *edit = KScoringEditor::createEditor(this, w);
  edit->setRule(e);
  edit->show();
  delete edit;
}

void KScoringManager::moveRuleAbove( KScoringRule *above, KScoringRule *below )
{
  int aindex = allRules.findRef( above );
  int bindex = allRules.findRef( below );
  if ( aindex <= 0 || bindex < 0 )
    return;
  if ( aindex < bindex )
    --bindex;
  allRules.take( aindex );
  allRules.insert( bindex, above );
}

void KScoringManager::moveRuleBelow( KScoringRule *below, KScoringRule *above )
{
  int bindex = allRules.findRef( below );
  int aindex = allRules.findRef( above );
  if ( bindex < 0 || bindex >= (int)allRules.count() - 1 || aindex < 0 )
    return;
  if ( bindex < aindex )
    --aindex;
  allRules.take( bindex );
  allRules.insert( aindex + 1, below );
}

void KScoringManager::editorReady()
{
  kDebug(5100) <<"emitting signal finishedEditing";
  save();
  emit finishedEditing();
}

KScoringRule* KScoringManager::copyRule(KScoringRule *r)
{
  KScoringRule *rule = new KScoringRule(*r);
  rule->setName(findUniqueName());
  addRuleInternal(rule);
  return rule;
}

void KScoringManager::applyRules(ScorableGroup* )
{
  kWarning(5100) <<"KScoringManager::applyRules(ScorableGroup* ) isn't implemented";
}

void KScoringManager::applyRules(ScorableArticle& article, const QString& group)
{
  setGroup(group);
  applyRules(article);
}

void KScoringManager::applyRules(ScorableArticle& a)
{
  Q3PtrListIterator<KScoringRule> it(isCacheValid()? ruleList : allRules);
  for( ; it.current(); ++it) {
    it.current()->applyRule(a);
  }
}

void KScoringManager::initCache(const QString& g)
{
  group = g;
  ruleList.clear();
  Q3PtrListIterator<KScoringRule> it(allRules);
  for (; it.current(); ++it) {
    if ( it.current()->matchGroup(group) ) {
      ruleList.append(it.current());
    }
  }
  kDebug(5100) <<"created cache for group" << group
                << "with" << ruleList.count() << "rules";
  setCacheValid(true);
}

void KScoringManager::setGroup(const QString& g)
{
  if (group != g) initCache(g);
}

bool KScoringManager::hasRulesForCurrentGroup()
{
  return ruleList.count() != 0;
}


QStringList KScoringManager::getRuleNames()
{
  QStringList l;
  Q3PtrListIterator<KScoringRule> it(allRules);
  for( ; it.current(); ++it) {
    l << it.current()->getName();
  }
  return l;
}

KScoringRule* KScoringManager::findRule(const QString& ruleName)
{
  Q3PtrListIterator<KScoringRule> it(allRules);
  for (; it.current(); ++it) {
    if ( it.current()->getName() == ruleName ) {
      return it;
    }
  }
  return 0;
}

bool KScoringManager::setCacheValid(bool v)
{
  bool res = cacheValid;
  cacheValid = v;
  return res;
}

QString KScoringManager::findUniqueName() const
{
  int nr = 0;
  QString ret;
  bool duplicated=false;

  while (nr < 99999999) {
    nr++;
    ret = i18n("rule %1", nr);

    duplicated=false;
    Q3PtrListIterator<KScoringRule> it(allRules);
    for( ; it.current(); ++it) {
      if (it.current()->getName() == ret) {
        duplicated = true;
        break;
      }
    }

    if (!duplicated)
      return ret;
  }

  return ret;
}

bool KScoringManager::hasFeature(int p)
{
  switch (p) {
    case ActionBase::SETSCORE: return canScores();
    case ActionBase::NOTIFY: return canNotes();
    case ActionBase::COLOR: return canColors();
    case ActionBase::MARKASREAD: return canMarkAsRead();
    default: return false;
  }
}

QStringList KScoringManager::getDefaultHeaders() const
{
  QStringList l;
  l.append("Subject");
  l.append("From");
  l.append("Date");
  l.append("Message-ID");
  return l;
}

void KScoringManager::pushRuleList()
{
  stack.push(allRules);
}

void KScoringManager::popRuleList()
{
  stack.pop(allRules);
}

void KScoringManager::removeTOS()
{
  stack.drop();
}

RuleStack::RuleStack()
{
}

RuleStack::~RuleStack()
{}

void RuleStack::push(Q3PtrList<KScoringRule>& l)
{
  kDebug(5100) <<"RuleStack::push pushing list with" << l.count() <<" rules";
  KScoringManager::ScoringRuleList *l1 = new KScoringManager::ScoringRuleList;
  for ( KScoringRule *r=l.first(); r != 0; r=l.next() ) {
    l1->append(new KScoringRule(*r));
  }
  stack.push(l1);
  kDebug(5100) <<"now there are" << stack.count() <<" lists on the stack";
}

void RuleStack::pop(Q3PtrList<KScoringRule>& l)
{
  top(l);
  drop();
  kDebug(5100) <<"RuleStack::pop pops list with" << l.count() <<" rules";
  kDebug(5100) <<"now there are" << stack.count() <<" lists on the stack";
}

void RuleStack::top(Q3PtrList<KScoringRule>& l)
{
  l.clear();
  KScoringManager::ScoringRuleList *l1 = stack.top();
  l = *l1;
}

void RuleStack::drop()
{
  kDebug(5100) <<"drop: now there are" << stack.count() <<" lists on the stack";
  stack.remove();
}


#include "kscoring.moc"
