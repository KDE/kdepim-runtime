/*
    kscoringeditor.cpp

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

#undef QT_NO_COMPAT

#include "kscoring.h"
#include "kscoringeditor.h"

#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kcolorcombo.h>
#include <kiconloader.h>
#include <kregexpeditorinterface.h>
#include <ktrader.h>
#include <kparts/componentfactory.h>
#include <kpushbutton.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qcheckbox.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qapplication.h>
#include <qtimer.h>
#include <q3hbox.h>
//Added by qt3to4:
#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QVBoxLayout>

// works for both ListBox and ComboBox
template <class T> static int setCurrentItem(T *box, const QString& s)
{
  int cnt = box->count();
  for (int i=0;i<cnt;++i) {
    if (box->text(i) == s) {
      box->setCurrentItem(i);
      return i;
    }
  }
  return -1;
}


//============================================================================
//
// class SingleConditionWidget (editor for one condition, used in ConditionEditWidget)
//
//============================================================================
SingleConditionWidget::SingleConditionWidget(KScoringManager *m,QWidget *p, const char *)
  : QFrame(p), manager(m)
{
  QBoxLayout *topL = new QVBoxLayout(this,5);
  QBoxLayout *firstRow = new QHBoxLayout(topL);
  neg = new QCheckBox(i18n("Not"),this);
  neg->setToolTip(i18n("Negate this condition"));
  firstRow->addWidget(neg);
  headers = new KComboBox(this);
  headers->insertStringList(manager->getDefaultHeaders());
  headers->setEditable( true );
  headers->setToolTip(i18n("Select the header to match this condition against"));
  firstRow->addWidget(headers,1);
  matches = new KComboBox(this);
  matches->insertStringList(KScoringExpression::conditionNames());
  matches->setToolTip(i18n("Select the type of match"));
  firstRow->addWidget(matches,1);
  connect( matches, SIGNAL( activated( int ) ), SLOT( toggleRegExpButton( int ) ) );
  QHBoxLayout *secondRow = new QHBoxLayout( topL );
  secondRow->setSpacing( 1 );
  expr = new KLineEdit( this );
  expr->setToolTip(i18n("The condition for the match"));
  // reserve space for at least 20 characters
  expr->setMinimumWidth(fontMetrics().maxWidth()*20);
  secondRow->addWidget( expr );
  regExpButton = new QPushButton( i18n("Edit..."), this );
  secondRow->addWidget( regExpButton );
  connect( regExpButton, SIGNAL( clicked() ), SLOT( showRegExpDialog() ) );

  // occupy at much width as possible
  setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed));
  setFrameStyle(Box | Sunken);
  setLineWidth(1);
}

SingleConditionWidget::~SingleConditionWidget()
{}

void SingleConditionWidget::setCondition(KScoringExpression *e)
{
  neg->setChecked(e->isNeg());
  headers->setCurrentText( e->getHeader() );
  setCurrentItem(matches,KScoringExpression::getNameForCondition(e->getCondition()));
  toggleRegExpButton( matches->currentItem() );
  expr->setText(e->getExpression());
}

KScoringExpression* SingleConditionWidget::createCondition() const
{
  QString head = headers->currentText();
  QString match = matches->currentText();
  int condType = KScoringExpression::getConditionForName(match);
  match = KScoringExpression::getTypeString(condType);
  QString cond = expr->text();
  QString negs = (neg->isChecked())?"1":"0";
  return new KScoringExpression(head,match,cond,negs);
}

void SingleConditionWidget::clear()
{
  neg->setChecked(false);
  expr->clear();
}

void SingleConditionWidget::toggleRegExpButton( int selected )
{
  bool isRegExp = KScoringExpression::MATCH == selected &&
      KScoringExpression::MATCHCS == selected &&
      !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty();
  regExpButton->setEnabled( isRegExp );
}

void SingleConditionWidget::showRegExpDialog()
{
  QDialog *editorDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor" );
  if ( editorDialog ) {
    KRegExpEditorInterface *editor = dynamic_cast<KRegExpEditorInterface *>( editorDialog );
    Q_ASSERT( editor ); // This should not fail!
    editor->setRegExp( expr->text() );
    editorDialog->exec();
    expr->setText( editor->regExp() );
  }
}

//============================================================================
//
// class ConditionEditWidget (the widget to edit the conditions of a rule)
//
//============================================================================
ConditionEditWidget::ConditionEditWidget(KScoringManager *m, QWidget *p, const char *n)
  : KWidgetLister(1,8,p,n), manager(m)
{
  // create one initial widget
  addWidgetAtEnd();
}

ConditionEditWidget::~ConditionEditWidget()
{}

QWidget* ConditionEditWidget::createWidget(QWidget *parent)
{
  return new SingleConditionWidget(manager,parent);
}

void ConditionEditWidget::clearWidget(QWidget *w)
{
  Q_ASSERT( w->isA("SingleConditionWidget") );
  SingleConditionWidget *sw = dynamic_cast<SingleConditionWidget*>(w);
  if (sw)
    sw->clear();
}

void ConditionEditWidget::slotEditRule(KScoringRule *rule)
{
  KScoringRule::ScoreExprList l;
  if (rule) l = rule->getExpressions();
  if (!rule || l.count() == 0) {
    slotClear();
  } else {
    setNumberOfShownWidgetsTo(l.count());
    KScoringExpression *e = l.first();
    SingleConditionWidget *scw = static_cast<SingleConditionWidget*>(mWidgetList.first());
    while (e && scw) {
      scw->setCondition(e);
      e = l.next();
      scw = static_cast<SingleConditionWidget*>(mWidgetList.next());
    }
  }
}

void ConditionEditWidget::updateRule(KScoringRule *rule)
{
  rule->cleanExpressions();
  for(QWidget *w = mWidgetList.first(); w; w = mWidgetList.next()) {
    if (! w->isA("SingleConditionWidget")) {
      kWarning(5100) << "there is a widget in ConditionEditWidget "
                      << "which isn't a SingleConditionWidget" << endl;
    } else {
      SingleConditionWidget *saw = dynamic_cast<SingleConditionWidget*>(w);
	  if (saw)
	    rule->addExpression(saw->createCondition());
    }
  }
}

//============================================================================
//
// class SingleActionWidget (editor for one action, used in ActionEditWidget)
//
//============================================================================
SingleActionWidget::SingleActionWidget(KScoringManager *m,QWidget *p, const char *n)
  : QWidget(p,n), notifyEditor(0), scoreEditor(0), colorEditor(0),manager(m)
{
  QHBoxLayout *topL = new QHBoxLayout(this,0,5);
  types = new KComboBox(this);
  types->setEditable(false);
  topL->addWidget(types);
  stack = new QStackedWidget(this);
  topL->addWidget(stack);

  dummyLabel = new QLabel(i18n("Select an action."), stack);
  stack->insertWidget(0,dummyLabel);

  // init widget stack and the types combo box
  int index = 1;
  types->insertItem(QString());
  QStringList l = ActionBase::userNames();
  for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
    QString name = *it;
    int feature = ActionBase::getTypeForUserName(name);
    if (manager->hasFeature(feature)) {
      types->insertItem(name);
      QWidget *w=0;
      switch (feature) {
        case ActionBase::SETSCORE:
          w = scoreEditor = new KIntSpinBox(stack);
          scoreEditor->setRange( -99999, 99999 );
          scoreEditor->setValue( 30 );
          break;
        case ActionBase::NOTIFY:
          w = notifyEditor = new KLineEdit(stack);
          break;
        case ActionBase::COLOR:
          w = colorEditor = new KColorCombo(stack);
          break;
        case ActionBase::MARKASREAD:
          w = new QLabel( stack ); // empty dummy
          break;
      }
      stack->insertWidget(index++,w);
    }
  }

  connect(types,SIGNAL(activated(int)),stack,SLOT(setCurrentIndex(int)));

  // raise the dummy label
  types->setCurrentItem(0);
  stack->setCurrentWidget(dummyLabel);
}

SingleActionWidget::~SingleActionWidget()
{
}

void SingleActionWidget::setAction(ActionBase *act)
{
  kDebug(5100) << "SingleActionWidget::setAction()" << endl;
  setCurrentItem(types,ActionBase::userName(act->getType()));
  int index = types->currentItem();
  stack->setCurrentIndex(index);
  switch (act->getType()) {
    case ActionBase::SETSCORE:
      scoreEditor->setValue(act->getValueString().toInt());
      break;
    case ActionBase::NOTIFY:
      notifyEditor->setText(act->getValueString());
      break;
    case ActionBase::COLOR:
      colorEditor->setColor(QColor(act->getValueString()));
      break;
    case ActionBase::MARKASREAD:
      // nothing
      break;
    default:
      kWarning(5100) << "unknown action type in SingleActionWidget::setAction()" << endl;
  }
}

ActionBase* SingleActionWidget::createAction() const
{
  // no action selected...
  if (types->currentText().isEmpty())
    return 0;

  int type = ActionBase::getTypeForUserName(types->currentText());
  switch (type) {
    case ActionBase::SETSCORE:
      return new ActionSetScore(scoreEditor->value());
    case ActionBase::NOTIFY:
      return new ActionNotify(notifyEditor->text());
    case ActionBase::COLOR:
      return new ActionColor(colorEditor->color().name());
    case ActionBase::MARKASREAD:
      return new ActionMarkAsRead();
    default:
      kWarning(5100) << "unknown action type in SingleActionWidget::getValue()" << endl;
      return 0;
  }
}

void SingleActionWidget::clear()
{
  if (scoreEditor) scoreEditor->setValue(0);
  if (notifyEditor) notifyEditor->clear();
  if (colorEditor) colorEditor->setCurrentItem(0);
  types->setCurrentItem(0);
  stack->setCurrentWidget(dummyLabel);
}

//============================================================================
//
// class ActionEditWidget (the widget to edit the actions of a rule)
//
//============================================================================
ActionEditWidget::ActionEditWidget(KScoringManager *m,QWidget *p, const char *n)
  : KWidgetLister(1,8,p,n), manager(m)
{
  // create one initial widget
  addWidgetAtEnd();
}

ActionEditWidget::~ActionEditWidget()
{}

QWidget* ActionEditWidget::createWidget( QWidget *parent )
{
  return new SingleActionWidget(manager,parent);
}

void ActionEditWidget::slotEditRule(KScoringRule *rule)
{
  KScoringRule::ActionList l;
  if (rule) l = rule->getActions();
  if (!rule || l.count() == 0) {
    slotClear();
  } else {
    setNumberOfShownWidgetsTo(l.count());
    ActionBase *act = l.first();
    SingleActionWidget *saw = static_cast<SingleActionWidget*>(mWidgetList.first());
    while (act && saw) {
      saw->setAction(act);
      act = l.next();
      saw = static_cast<SingleActionWidget*>(mWidgetList.next());
    }
  }
}

void ActionEditWidget::updateRule(KScoringRule *rule)
{
  rule->cleanActions();
  for(QWidget *w = mWidgetList.first(); w; w = mWidgetList.next()) {
    if (! w->isA("SingleActionWidget")) {
      kWarning(5100) << "there is a widget in ActionEditWidget "
                      << "which isn't a SingleActionWidget" << endl;
    } else {
      SingleActionWidget *saw = dynamic_cast<SingleActionWidget*>(w);
	  if (saw)
	  {
	  	ActionBase *act = saw->createAction();
        if (act)
          rule->addAction(act);
      }
    }
  }
}

void ActionEditWidget::clearWidget(QWidget *w)
{
  Q_ASSERT( w->isA("SingleActionWidget") );
  SingleActionWidget *sw = dynamic_cast<SingleActionWidget*>(w);
  if (sw)
    sw->clear();
}

//============================================================================
//
// class RuleEditWidget (the widget to edit one rule)
//
//============================================================================
RuleEditWidget::RuleEditWidget(KScoringManager *m,QWidget *p, const char *n)
  : QWidget(p,n), dirty(false), manager(m), oldRuleName(QString())
{
  kDebug(5100) << "RuleEditWidget::RuleEditWidget()" << endl;
  if ( !n ) setName( "RuleEditWidget" );
  QVBoxLayout *topLayout = new QVBoxLayout( this, 5, KDialog::spacingHint() );

  //------------- Name, Servers, Groups ---------------------
  Q3GroupBox *groupB = new Q3GroupBox(i18n("Properties"),this);
  topLayout->addWidget(groupB);
  QGridLayout* groupL = new QGridLayout(groupB, 6,2, 8,5);
  groupL->addRowSpacing(0, fontMetrics().lineSpacing()-4);

  // name
  ruleNameEdit = new KLineEdit( groupB );
  groupL->addWidget( ruleNameEdit, 1, 1 );
  QLabel *ruleNameLabel = new QLabel(ruleNameEdit, i18n("&Name:"), groupB, "ruleNameLabel");
  groupL->addWidget( ruleNameLabel, 1, 0 );

  // groups
  groupsEdit = new KLineEdit( groupB );
  groupL->addWidget( groupsEdit, 2, 1 );
  QLabel *groupsLabel = new QLabel(groupsEdit, i18n("&Groups:"), groupB, "groupsLabel");
  groupL->addWidget( groupsLabel, 2, 0 );

  QPushButton *groupsBtn = new QPushButton(i18n("A&dd Group"), groupB);
  connect(groupsBtn,SIGNAL(clicked()),SLOT(slotAddGroup()));
  groupL->addWidget( groupsBtn, 3, 0 );

  groupsBox = new KComboBox( false, groupB );
  groupsBox->setDuplicatesEnabled(false);
  groupsBox->insertStringList(manager->getGroups());
  groupsBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  groupL->addWidget( groupsBox, 3, 1 );

  // expires
  expireCheck = new QCheckBox(i18n("&Expire rule automatically"), groupB);
  groupL->addMultiCellWidget( expireCheck, 4,4, 0,1 );
  expireEdit = new KIntSpinBox( groupB );
  expireEdit->setRange( 1, 9999 );
  expireEdit->setValue( 30 );
  connect(expireEdit, SIGNAL(valueChanged(int)), SLOT(slotExpireEditChanged(int)));
  groupL->addWidget( expireEdit, 5, 1 );
  expireLabel = new QLabel(expireEdit, i18n("&Rule is valid for:"), groupB, "expireLabel");
  groupL->addWidget( expireLabel, 5, 0 );
  expireLabel->setEnabled(false);
  expireEdit->setEnabled(false);

  connect(expireCheck, SIGNAL(toggled(bool)), expireLabel, SLOT(setEnabled(bool)));
  connect(expireCheck, SIGNAL(toggled(bool)), expireEdit, SLOT(setEnabled(bool)));

  //------------- Conditions ---------------------
  Q3GroupBox *groupConds = new Q3GroupBox(i18n("Conditions"), this);
  topLayout->addWidget(groupConds);
  QGridLayout *condL = new QGridLayout(groupConds, 3,2, 8,5);

  condL->addRowSpacing(0, fontMetrics().lineSpacing()-4);

  Q3ButtonGroup *buttonGroup = new Q3ButtonGroup(groupConds);
  buttonGroup->hide();
  linkModeAnd = new QRadioButton(i18n("Match a&ll conditions"), groupConds);
  buttonGroup->insert(linkModeAnd);
  condL->addWidget(linkModeAnd, 1,0);
  linkModeOr = new QRadioButton(i18n("Matc&h any condition"), groupConds);
  buttonGroup->insert(linkModeOr);
  condL->addWidget(linkModeOr, 1,1);
  linkModeAnd->setChecked(true);

  condEditor = new ConditionEditWidget(manager,groupConds);
  condL->addMultiCellWidget(condEditor, 2,2, 0,1);
  connect(condEditor,SIGNAL(widgetRemoved()),this,SLOT(slotShrink()));

  //------------- Actions ---------------------
  Q3GroupBox *groupActions = new Q3GroupBox(i18n("Actions"), this);
  topLayout->addWidget(groupActions);
  QBoxLayout *actionL = new QVBoxLayout(groupActions,8,5);
  actionL->addSpacing(fontMetrics().lineSpacing()-4);
  actionEditor = new ActionEditWidget(manager,groupActions);
  actionL->addWidget(actionEditor);
  connect(actionEditor,SIGNAL(widgetRemoved()),this,SLOT(slotShrink()));

  topLayout->addStretch(1);

  kDebug(5100) << "constructed RuleEditWidget" << endl;
}

RuleEditWidget::~RuleEditWidget()
{
}

void RuleEditWidget::slotEditRule(const QString& ruleName)
{
  kDebug(5100) << "RuleEditWidget::slotEditRule(" << ruleName << ")" << endl;
//   // first update the old rule if there is one
//   kDebug(5100) << "let see if we have a rule with name " << oldRuleName << endl;
//   KScoringRule *rule;
//   if (!oldRuleName.isNull() && oldRuleName != ruleName) {
//     rule = manager->findRule(oldRuleName);
//     if (rule) {
//       kDebug(5100) << "updating rule " << rule->getName() << endl;
//       updateRule(rule);
//     }
//   }

  KScoringRule* rule = manager->findRule(ruleName);
  if (!rule) {
    kDebug(5100) << "no rule for ruleName " << ruleName << endl;
    clearContents();
    return;
  }
  oldRuleName = rule->getName();
  ruleNameEdit->setText(rule->getName());
  groupsEdit->setText(rule->getGroups().join(";"));

  bool b = rule->getExpireDate().isValid();
  expireCheck->setChecked(b);
  expireEdit->setEnabled(b);
  expireLabel->setEnabled(b);
  if (b)
    expireEdit->setValue(QDate::currentDate().daysTo(rule->getExpireDate()));
  else
    expireEdit->setValue(30);
  if (rule->getLinkMode() == KScoringRule::AND) {
    linkModeAnd->setChecked(true);
  }
  else {
    linkModeOr->setChecked(true);
  }

  condEditor->slotEditRule(rule);
  actionEditor->slotEditRule(rule);

  kDebug(5100) << "RuleEditWidget::slotEditRule() ready" << endl;
}

void RuleEditWidget::clearContents()
{
  ruleNameEdit->setText("");
  groupsEdit->setText("");
  expireCheck->setChecked(false);
  expireEdit->setValue(30);
  expireEdit->setEnabled(false);
  condEditor->slotEditRule(0);
  actionEditor->slotEditRule(0);
  oldRuleName.clear();
}

void RuleEditWidget::updateRule(KScoringRule *rule)
{
  oldRuleName.clear();
  QString groups = groupsEdit->text();
  if (groups.isEmpty())
    rule->setGroups(QStringList(".*"));
  else
    rule->setGroups(groups.split(";",QString::SkipEmptyParts));
  bool b = expireCheck->isChecked();
  if (b)
    rule->setExpireDate(QDate::currentDate().addDays(expireEdit->value()));
  else
    rule->setExpireDate(QDate());
  actionEditor->updateRule(rule);
  rule->setLinkMode(linkModeAnd->isChecked()?KScoringRule::AND:KScoringRule::OR);
  condEditor->updateRule(rule);
  if (rule->getName() != ruleNameEdit->text())
    manager->setRuleName(rule,ruleNameEdit->text());
}

void RuleEditWidget::updateRule()
{
  KScoringRule *rule = manager->findRule(oldRuleName);
  if (rule) updateRule(rule);
}

void RuleEditWidget::slotAddGroup()
{
  QString grp = groupsBox->currentText();
  if ( grp.isEmpty() )
      return;
  QString txt = groupsEdit->text().trimmed();
  if ( txt == ".*" || txt.isEmpty() ) groupsEdit->setText(grp);
  else groupsEdit->setText(txt + ';' + grp);
}

void RuleEditWidget::setDirty()
{
  kDebug(5100) << "RuleEditWidget::setDirty()" << endl;
  if (dirty) return;
  dirty = true;
}

void RuleEditWidget::slotShrink()
{
  emit(shrink());
}

void RuleEditWidget::slotExpireEditChanged(int value)
{
  expireEdit->setSuffix(i18n(" day", " days", value));
}

//============================================================================
//
// class RuleListWidget (the widget for managing a list of rules)
//
//============================================================================
RuleListWidget::RuleListWidget(KScoringManager *m, bool standalone, QWidget *p, const char *n)
  : QWidget(p,n), alone(standalone), manager(m)
{
  kDebug(5100) << "RuleListWidget::RuleListWidget()" << endl;
  if (!n) setName("RuleListWidget");
  QVBoxLayout *topL = new QVBoxLayout(this,standalone? 0:5,KDialog::spacingHint());
  ruleList = new KListBox(this);
  if (standalone) {
    connect(ruleList,SIGNAL(doubleClicked(Q3ListBoxItem*)),
            this,SLOT(slotEditRule(Q3ListBoxItem*)));
    connect(ruleList,SIGNAL(returnPressed(Q3ListBoxItem*)),
            this,SLOT(slotEditRule(Q3ListBoxItem*)));
  }
  connect(ruleList, SIGNAL(currentChanged(Q3ListBoxItem*)),
          this, SLOT(slotRuleSelected(Q3ListBoxItem*)));
  topL->addWidget(ruleList);

  QHBoxLayout *btnL = new QHBoxLayout( topL, KDialog::spacingHint() );
  mRuleUp = new QPushButton( this );
  mRuleUp->setPixmap( BarIcon( "up", K3Icon::SizeSmall ) );
  mRuleUp->setToolTip( i18n("Move rule up") );
  btnL->addWidget( mRuleUp );
  connect( mRuleUp, SIGNAL( clicked() ), SLOT( slotRuleUp() ) );
  mRuleDown = new QPushButton( this );
  mRuleDown->setPixmap( BarIcon( "down", K3Icon::SizeSmall ) );
  mRuleDown->setToolTip( i18n("Move rule down") );
  btnL->addWidget( mRuleDown );
  connect( mRuleDown, SIGNAL( clicked() ), SLOT( slotRuleDown() ) );

  btnL = new QHBoxLayout( topL, KDialog::spacingHint() );
  editRule=0L;
  newRule = new QPushButton(this);
  newRule->setPixmap( BarIcon( "filenew", K3Icon::SizeSmall ) );
  newRule->setToolTip(i18n("New rule")),
  btnL->addWidget(newRule);
  connect(newRule, SIGNAL(clicked()), this, SLOT(slotNewRule()));
  // if we're standalone, we need an additional edit button
  if (standalone) {
    editRule = new QPushButton(this);
    editRule->setIconSet( BarIconSet("edit", K3Icon::SizeSmall) );
    editRule->setToolTip(i18n("Edit rule"));
    btnL->addWidget(editRule);
    connect(editRule,SIGNAL(clicked()),this,SLOT(slotEditRule()));
  }
  delRule = new QPushButton(this);
  delRule->setIconSet( BarIconSet( "editdelete", K3Icon::SizeSmall ) );
  delRule->setToolTip(i18n("Remove rule"));
  btnL->addWidget(delRule);
  connect(delRule, SIGNAL(clicked()), this, SLOT(slotDelRule()));
  copyRule = new QPushButton(this);
  copyRule->setIconSet(BarIconSet("editcopy", K3Icon::SizeSmall));
  copyRule->setToolTip(i18n("Copy rule"));
  btnL->addWidget(copyRule);
  connect(copyRule, SIGNAL(clicked()), this, SLOT(slotCopyRule()));

  // the group filter
  QBoxLayout *filterL = new QVBoxLayout(topL,KDialog::spacingHint());
  KComboBox *filterBox = new KComboBox(this);
  QStringList l = m->getGroups();
  filterBox->insertItem(i18n("<all groups>"));
  filterBox->insertStringList(l);
  filterBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  connect(filterBox,SIGNAL(activated(const QString&)),
          this,SLOT(slotGroupFilter(const QString&)));
  slotGroupFilter(i18n("<all groups>"));
  QLabel *lab = new QLabel(filterBox,i18n("Sho&w only rules for group:"),this);
  filterL->addWidget(lab);
  filterL->addWidget(filterBox);

  connect(manager,SIGNAL(changedRules()),
          this,SLOT(updateRuleList()));
  connect(manager,SIGNAL(changedRuleName(const QString&,const QString&)),
          this,SLOT(slotRuleNameChanged(const QString&,const QString&)));

  updateRuleList();
  updateButton();
}

RuleListWidget::~RuleListWidget()
{
}

void RuleListWidget::updateButton()
{
  bool state = ruleList->count() > 0;
  if(editRule)
    editRule->setEnabled(state);
  delRule->setEnabled(state);
  copyRule->setEnabled(state);

  Q3ListBoxItem *item = ruleList->item( ruleList->currentItem() );
  if ( item ) {
    mRuleUp->setEnabled( item->prev() != 0 );
    mRuleDown->setEnabled( item->next() != 0 );
  }
}

void RuleListWidget::updateRuleList()
{
  emit leavingRule();
  kDebug(5100) << "RuleListWidget::updateRuleList()" << endl;
  QString curr = ruleList->currentText();
  ruleList->clear();
  if (group == i18n("<all groups>")) {
    QStringList l = manager->getRuleNames();
    ruleList->insertStringList(l);
  } else {
    KScoringManager::ScoringRuleList l = manager->getAllRules();
    for (KScoringRule* rule = l.first(); rule; rule = l.next() ) {
      if (rule->matchGroup(group)) ruleList->insertItem(rule->getName());
    }
  }
  int index = setCurrentItem(ruleList,curr);
  if (index <0) {
    ruleList->setCurrentItem(0);
    slotRuleSelected(ruleList->currentText());
  }
  else {
    slotRuleSelected(curr);
  }
}

void RuleListWidget::updateRuleList(const KScoringRule *rule)
{
  kDebug(5100) << "RuleListWidget::updateRuleList(" << rule->getName() << ")" << endl;
  QString name = rule->getName();
  updateRuleList();
  slotRuleSelected(name);
}

void RuleListWidget::slotRuleNameChanged(const QString& oldName, const QString& newName)
{
  int ind = ruleList->currentItem();
  for (uint i=0;i<ruleList->count();++i)
    if (ruleList->text(i) == oldName) {
      ruleList->changeItem(newName,i);
      ruleList->setCurrentItem(ind);
      return;
    }
}

void RuleListWidget::slotEditRule(const QString& s)
{
  emit ruleEdited(s);
}

void RuleListWidget::slotEditRule()
{
  if (ruleList->currentItem() >= 0) {
    emit ruleEdited(ruleList->currentText());
  }
  else if (ruleList->count() == 0)
    emit ruleEdited(QString());
}

void RuleListWidget::slotEditRule(Q3ListBoxItem* item)
{
  slotEditRule(item->text());
}

void RuleListWidget::slotGroupFilter(const QString& s)
{
  group = s;
  updateRuleList();
}

void RuleListWidget::slotRuleSelected(const QString& ruleName)
{
  emit leavingRule();
  kDebug(5100) << "RuleListWidget::slotRuleSelected(" << ruleName << ")" << endl;
  if (ruleName != ruleList->currentText()) {
    setCurrentItem(ruleList,ruleName);
  }
  updateButton();
  emit ruleSelected(ruleName);
}

void RuleListWidget::slotRuleSelected(Q3ListBoxItem *item)
{
  if (!item) return;
  QString ruleName = item->text();
  slotRuleSelected(ruleName);
}

void RuleListWidget::slotRuleSelected(int index)
{
  uint idx = index;
  if (idx >= ruleList->count()) return;
  QString ruleName = ruleList->text(index);
  slotRuleSelected(ruleName);
}

void RuleListWidget::slotNewRule()
{
  emit leavingRule();
  KScoringRule *rule = manager->addRule();
  updateRuleList(rule);
  if (alone) slotEditRule(rule->getName());
  updateButton();
}

void RuleListWidget::slotDelRule()
{
  KScoringRule *rule = manager->findRule(ruleList->currentText());
  if (rule)
    manager->deleteRule(rule);
  // goto the next rule
  if (!alone) slotEditRule();
  updateButton();
}

void RuleListWidget::slotCopyRule()
{
  emit leavingRule();
  QString ruleName = ruleList->currentText();
  KScoringRule *rule = manager->findRule(ruleName);
  if (rule) {
    KScoringRule *nrule = manager->copyRule(rule);
    updateRuleList(nrule);
    slotEditRule(nrule->getName());
  }
  updateButton();
}

void RuleListWidget::slotRuleUp()
{
  KScoringRule *rule = 0, *below = 0;
  Q3ListBoxItem *item = ruleList->item( ruleList->currentItem() );
  if ( item ) {
    rule = manager->findRule( item->text() );
    item = item->prev();
    if ( item )
      below = manager->findRule( item->text() );
  }
  if ( rule && below )
    manager->moveRuleAbove( rule, below );
  updateRuleList();
  updateButton();
}

void RuleListWidget::slotRuleDown()
{
  KScoringRule *rule = 0, *above = 0;
  Q3ListBoxItem *item = ruleList->item( ruleList->currentItem() );
  if ( item ) {
    rule = manager->findRule( item->text() );
    item = item->next();
    if ( item )
      above = manager->findRule( item->text() );
  }
  if ( rule && above )
    manager->moveRuleBelow( rule, above );
  updateRuleList();
  updateButton();
}

//============================================================================
//
// class KScoringEditor (the score edit dialog)
//
//============================================================================
KScoringEditor* KScoringEditor::scoreEditor = 0;

KScoringEditor::KScoringEditor(KScoringManager* m,
                               QWidget *parent, const char *name)
  : KDialogBase(parent,name,false,i18n("Rule Editor"),Ok|Apply|Cancel,Ok,true), manager(m)
{
  manager->pushRuleList();
  if (!scoreEditor) scoreEditor = this;
  kDebug(5100) << "KScoringEditor::KScoringEditor()" << endl;
  if (!name) setName("KScoringEditor");
  // the left side gives an overview about all rules, the right side
  // shows a detailed view of an selected rule
  QWidget *w = new QWidget(this);
  setMainWidget(w);
  QHBoxLayout *hbl = new QHBoxLayout(w,0,spacingHint());
  ruleLister = new RuleListWidget(manager,false,w);
  hbl->addWidget(ruleLister);
  ruleEditor = new RuleEditWidget(manager,w);
  hbl->addWidget(ruleEditor);
  connect(ruleLister,SIGNAL(ruleSelected(const QString&)),
          ruleEditor, SLOT(slotEditRule(const QString&)));
  connect(ruleLister, SIGNAL(leavingRule()),
          ruleEditor, SLOT(updateRule()));
  connect(ruleEditor, SIGNAL(shrink()), SLOT(slotShrink()));
  connect(this,SIGNAL(finished()),SLOT(slotFinished()));
  ruleLister->slotRuleSelected(0);
  resize(550, sizeHint().height());
}

void KScoringEditor::setDirty()
{
  KPushButton *applyBtn = actionButton(Apply);
  applyBtn->setEnabled(true);
}

KScoringEditor::~KScoringEditor()
{
  scoreEditor = 0;
}

KScoringEditor* KScoringEditor::createEditor(KScoringManager* m,
                                             QWidget *parent, const char *name)
{
  if (scoreEditor) return scoreEditor;
  else return new KScoringEditor(m,parent,name);
}

void KScoringEditor::setRule(KScoringRule* r)
{
  kDebug(5100) << "KScoringEditor::setRule(" << r->getName() << ")" << endl;
  QString ruleName = r->getName();
  ruleLister->slotRuleSelected(ruleName);
}

void KScoringEditor::slotShrink()
{
  QTimer::singleShot(5, this, SLOT(slotDoShrink()));
}

void KScoringEditor::slotDoShrink()
{
  updateGeometry();
  QApplication::sendPostedEvents();
  resize(width(),sizeHint().height());
}

void KScoringEditor::slotApply()
{
  QString ruleName = ruleLister->currentRule();
  KScoringRule *rule = manager->findRule(ruleName);
  if (rule) {
    ruleEditor->updateRule(rule);
    ruleLister->updateRuleList(rule);
  }
  manager->removeTOS();
  manager->pushRuleList();
}

void KScoringEditor::slotOk()
{
  slotApply();
  manager->removeTOS();
  KDialogBase::slotOk();
  manager->editorReady();
}

void KScoringEditor::slotCancel()
{
  manager->popRuleList();
  KDialogBase::slotCancel();
}

void KScoringEditor::slotFinished()
{
  delayedDestruct();
}

//============================================================================
//
// class KScoringEditorWidgetDialog (a dialog for the KScoringEditorWidget)
//
//============================================================================
KScoringEditorWidgetDialog::KScoringEditorWidgetDialog(KScoringManager *m, const QString& r, QWidget *p, const char *n)
  : KDialogBase(p,n,true,i18n("Edit Rule"),
                KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Close,
                KDialogBase::Ok,true),
    manager(m), ruleName(r)
{
  QFrame *f = makeMainWidget();
  QBoxLayout *topL = new QVBoxLayout(f);
  ruleEditor = new RuleEditWidget(manager,f);
  connect(ruleEditor, SIGNAL(shrink()), SLOT(slotShrink()));
  topL->addWidget(ruleEditor);
  ruleEditor->slotEditRule(ruleName);
  resize(0,0);
}

void KScoringEditorWidgetDialog::slotApply()
{
  KScoringRule *rule = manager->findRule(ruleName);
  if (rule) {
    ruleEditor->updateRule(rule);
    ruleName = rule->getName();
  }
}

void KScoringEditorWidgetDialog::slotOk()
{
  slotApply();
  KDialogBase::slotOk();
}

void KScoringEditorWidgetDialog::slotShrink()
{
  QTimer::singleShot(5, this, SLOT(slotDoShrink()));
}

void KScoringEditorWidgetDialog::slotDoShrink()
{
  updateGeometry();
  QApplication::sendPostedEvents();
  resize(width(),sizeHint().height());
}

//============================================================================
//
// class KScoringEditorWidget (a reusable widget for config dialog...)
//
//============================================================================
KScoringEditorWidget::KScoringEditorWidget(KScoringManager *m,QWidget *p, const char *n)
  : QWidget(p,n), manager(m)
{
  QBoxLayout *topL = new QVBoxLayout(this);
  ruleLister = new RuleListWidget(manager,true,this);
  topL->addWidget(ruleLister);
  connect(ruleLister,SIGNAL(ruleEdited(const QString&)),
          this,SLOT(slotRuleEdited(const QString &)));
}

KScoringEditorWidget::~KScoringEditorWidget()
{
  manager->editorReady();
}

void KScoringEditorWidget::slotRuleEdited(const QString& ruleName)
{
  KScoringEditorWidgetDialog dlg(manager,ruleName,this);
  dlg.exec();
  ruleLister->updateRuleList();
}

#include "kscoringeditor.moc"
