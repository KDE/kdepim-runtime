/*
  kscoringeditor.cpp

  Copyright (c) 2001 Mathias Waack <mathias@atoll-net.de>
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

#include "kscoringeditor.h"
#include "kscoring.h"

#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kcolorcombo.h>
#include <kiconloader.h>
#include <kregexpeditorinterface.h>
#include <kservicetypeprofile.h>
#include <kparts/componentfactory.h>
#include <kpushbutton.h>

#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QCheckBox>
#include <QRadioButton>
#include <QApplication>
#include <QTimer>
#include <QButtonGroup>
#include <QGroupBox>
#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QVBoxLayout>

using namespace KPIM;

static int setCurrentItem( K3ListBox *box, const QString &s )
{
  int cnt = box->count();
  for ( int i=0; i<cnt; ++i ) {
    if ( box->text( i ) == s ) {
      box->setCurrentItem( i );
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
SingleConditionWidget::SingleConditionWidget( KScoringManager *m, QWidget *p, const char * )
  : QFrame( p ), manager( m )
{
  QBoxLayout *topL = new QVBoxLayout( this );
  topL->setMargin( 5 );
  QBoxLayout *firstRow = new QHBoxLayout();
  topL->addItem( firstRow );
  neg = new QCheckBox( i18n( "Not" ), this );
  neg->setToolTip( i18n( "Negate this condition" ) );
  firstRow->addWidget( neg );
  headers = new KComboBox( this );
  headers->addItems( manager->getDefaultHeaders() );
  headers->setEditable( true );
  headers->setToolTip( i18n( "Select the header to match this condition against" ) );
  firstRow->addWidget( headers, 1 );
  matches = new KComboBox( this );
  matches->addItems( KScoringExpression::conditionNames() );
  matches->setToolTip( i18n( "Select the type of match" ) );
  firstRow->addWidget( matches, 1 );
  connect( matches, SIGNAL( activated( int ) ), SLOT( toggleRegExpButton( int ) ) );
  QHBoxLayout *secondRow = new QHBoxLayout();
  secondRow->setSpacing( 1 );
  topL->addItem( secondRow );

  expr = new KLineEdit( this );
  expr->setToolTip( i18n( "The condition for the match" ) );
  // reserve space for at least 20 characters
  expr->setMinimumWidth( fontMetrics().maxWidth() * 20 );
  secondRow->addWidget( expr );
  regExpButton = new QPushButton( i18n( "Edit..." ), this );
  secondRow->addWidget( regExpButton );
  connect( regExpButton, SIGNAL( clicked() ), SLOT( showRegExpDialog() ) );

  // occupy at much width as possible
  setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
  setFrameStyle( Box | Sunken );
  setLineWidth( 1 );
}

SingleConditionWidget::~SingleConditionWidget()
{}

void SingleConditionWidget::setCondition( KScoringExpression *e )
{
  neg->setChecked( e->isNeg() );
  headers->setItemText( headers->currentIndex(), e->getHeader() );
  matches->setCurrentItem( KScoringExpression::getNameForCondition( e->getCondition() ) );
  toggleRegExpButton( matches->currentIndex() );
  expr->setText( e->getExpression() );
}

KScoringExpression *SingleConditionWidget::createCondition() const
{
  QString head = headers->currentText();
  QString match = matches->currentText();
  int condType = KScoringExpression::getConditionForName( match );
  match = KScoringExpression::getTypeString( condType );
  QString cond = expr->text();
  QString negs = ( neg->isChecked() ) ? "1" : "0";
  return new KScoringExpression( head, match, cond, negs );
}

void SingleConditionWidget::clear()
{
  neg->setChecked( false );
  expr->clear();
}

void SingleConditionWidget::toggleRegExpButton( int selected )
{
  bool isRegExp = ( KScoringExpression::MATCH == selected ||
                    KScoringExpression::MATCHCS == selected ) &&
                  !KServiceTypeTrader::self()->query( "KRegExpEditor/KRegExpEditor" ).isEmpty();
  regExpButton->setEnabled( isRegExp );
}

void SingleConditionWidget::showRegExpDialog()
{
  QDialog *editorDialog =
    KParts::ComponentFactory::createPartInstanceFromQuery<QDialog>(
      "KRegExpEditor/KRegExpEditor", QString() );
  if ( editorDialog ) {
    KRegExpEditorInterface *editor = qobject_cast<KRegExpEditorInterface *>( editorDialog );
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
ConditionEditWidget::ConditionEditWidget( KScoringManager *m, QWidget *p, const char *n )
  : KWidgetLister( 1, 8, p, n ), manager( m )
{
  // create one initial widget
  addWidgetAtEnd();
}

ConditionEditWidget::~ConditionEditWidget()
{}

QWidget *ConditionEditWidget::createWidget( QWidget *parent )
{
  return new SingleConditionWidget( manager, parent );
}

void ConditionEditWidget::clearWidget( QWidget *w )
{
  SingleActionWidget *sw =  qobject_cast<SingleActionWidget *>(w);
  Q_ASSERT( w );
  if ( sw ) {
    sw->clear();
  }
}

void ConditionEditWidget::slotEditRule( KScoringRule *rule )
{
  KScoringRule::ScoreExprList l;
  if ( rule ) {
    l = rule->getExpressions();
  }
  if ( !rule || l.count() == 0 ) {
    slotClear();
  } else {
    setNumberOfShownWidgetsTo( l.count() );
    KScoringExpression *e = l.first();
    SingleConditionWidget *scw = static_cast<SingleConditionWidget*>( mWidgetList.first() );
    while ( e && scw ) {
      scw->setCondition( e );
      e = l.next();
      scw = static_cast<SingleConditionWidget*>( mWidgetList.next() );
    }
  }
}

void ConditionEditWidget::updateRule( KScoringRule *rule )
{
  rule->cleanExpressions();
  for ( QWidget *w = mWidgetList.first(); w; w = mWidgetList.next() ) {
      if ( QString( w->metaObject()->className() ) != "SingleConditionWidget" ) {
      kWarning(5100) <<"there is a widget in ConditionEditWidget"
                     << "which isn't a SingleConditionWidget";
    } else {
      SingleConditionWidget *saw = dynamic_cast<SingleConditionWidget*>( w );
      if ( saw ) {
        rule->addExpression( saw->createCondition() );
      }
    }
  }
}

//============================================================================
//
// class SingleActionWidget (editor for one action, used in ActionEditWidget)
//
//============================================================================
SingleActionWidget::SingleActionWidget( KScoringManager *m, QWidget *p, const char *n )
  : QWidget( p ), notifyEditor( 0 ), scoreEditor( 0 ),
    colorEditor( 0 ), manager( m )
{
  setObjectName( n );
  QHBoxLayout *topL = new QHBoxLayout( this );
  topL->setMargin( 0 );
  topL->setSpacing( 5 );

  types = new KComboBox( this );
  types->setEditable( false );
  topL->addWidget( types );
  stack = new QStackedWidget( this );
  topL->addWidget( stack );

  dummyLabel = new QLabel( i18n( "Select an action." ), stack );
  stack->insertWidget( 0, dummyLabel );

  // init widget stack and the types combo box
  int index = 1;
  types->addItem( QString() );
  QStringList l = ActionBase::userNames();
  for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
    QString name = *it;
    int feature = ActionBase::getTypeForUserName( name );
    if ( manager->hasFeature( feature ) ) {
      types->addItem( name );
      QWidget *w = 0;
      switch( feature ) {
      case ActionBase::SETSCORE:
        w = scoreEditor = new KIntSpinBox( stack );
        scoreEditor->setRange( -99999, 99999 );
        scoreEditor->setValue( 30 );
        break;
      case ActionBase::NOTIFY:
        w = notifyEditor = new KLineEdit( stack );
        break;
      case ActionBase::COLOR:
        w = colorEditor = new KColorCombo( stack );
        break;
      case ActionBase::MARKASREAD:
        w = new QLabel( stack ); // empty dummy
        break;
      }
      if ( w ) {
        stack->insertWidget( index++, w );
      }
    }
  }

  connect( types, SIGNAL(activated(int)), stack, SLOT(setCurrentIndex(int)) );

  // raise the dummy label
  types->setCurrentIndex( 0 );
  stack->setCurrentWidget( dummyLabel );
}

SingleActionWidget::~SingleActionWidget()
{
}

void SingleActionWidget::setAction( ActionBase *act )
{
  kDebug(5100) <<"SingleActionWidget::setAction()";

  int index = types->currentIndex();
  types->setItemText( index, ActionBase::userName( act->getType() ) );

  stack->setCurrentIndex( index );
  switch( act->getType() ) {
  case ActionBase::SETSCORE:
    scoreEditor->setValue( act->getValueString().toInt() );
    break;
  case ActionBase::NOTIFY:
    notifyEditor->setText( act->getValueString() );
    break;
  case ActionBase::COLOR:
    colorEditor->setColor( QColor( act->getValueString() ) );
    break;
  case ActionBase::MARKASREAD:
    // nothing
    break;
  default:
    kWarning(5100) <<"unknown action type in SingleActionWidget::setAction()";
  }
}

ActionBase *SingleActionWidget::createAction() const
{
  // no action selected...
  if ( types->currentText().isEmpty() ) {
    return 0;
  }

  int type = ActionBase::getTypeForUserName( types->currentText() );
  switch ( type ) {
  case ActionBase::SETSCORE:
    return new ActionSetScore( scoreEditor->value() );
  case ActionBase::NOTIFY:
    return new ActionNotify( notifyEditor->text() );
  case ActionBase::COLOR:
    return new ActionColor( colorEditor->color().name() );
  case ActionBase::MARKASREAD:
    return new ActionMarkAsRead();
  default:
    kWarning(5100) <<"unknown action type in SingleActionWidget::getValue()";
    return 0;
  }
}

void SingleActionWidget::clear()
{
  if ( scoreEditor ) {
    scoreEditor->setValue( 0 );
  }
  if ( notifyEditor ) {
    notifyEditor->clear();
  }
  if ( colorEditor ) {
    colorEditor->setCurrentIndex( 0 );
  }

  types->setCurrentIndex( 0 );
  stack->setCurrentWidget( dummyLabel );
}

//============================================================================
//
// class ActionEditWidget (the widget to edit the actions of a rule)
//
//============================================================================
ActionEditWidget::ActionEditWidget( KScoringManager *m, QWidget *p, const char *n )
  : KWidgetLister( 1, 8, p, n ), manager( m )
{
  // create one initial widget
  addWidgetAtEnd();
}

ActionEditWidget::~ActionEditWidget()
{}

QWidget *ActionEditWidget::createWidget( QWidget *parent )
{
  return new SingleActionWidget( manager, parent );
}

void ActionEditWidget::slotEditRule( KScoringRule *rule )
{
  KScoringRule::ActionList l;
  if ( rule ) {
    l = rule->getActions();
  }
  if ( !rule || l.count() == 0 ) {
    slotClear();
  } else {
    setNumberOfShownWidgetsTo( l.count() );
    ActionBase *act = l.first();
    SingleActionWidget *saw = static_cast<SingleActionWidget*>( mWidgetList.first() );
    while ( act && saw ) {
      saw->setAction( act );
      act = l.next();
      saw = static_cast<SingleActionWidget*>( mWidgetList.next() );
    }
  }
}

void ActionEditWidget::updateRule( KScoringRule *rule )
{
  rule->cleanActions();
  for ( QWidget *w = mWidgetList.first(); w; w = mWidgetList.next() ) {
    if ( QString( w->metaObject()->className() ) != "SingleActionWidget" ) {
      kWarning(5100) <<"there is a widget in ActionEditWidget"
                     << "which isn't a SingleActionWidget";
    } else {
      SingleActionWidget *saw = dynamic_cast<SingleActionWidget*>( w );
      if (saw) {
        ActionBase *act = saw->createAction();
        if ( act ) {
          rule->addAction( act );
        }
      }
    }
  }
}

void ActionEditWidget::clearWidget( QWidget *w )
{
  SingleActionWidget *sw =  qobject_cast<SingleActionWidget *>(w);
  Q_ASSERT( w );
  if ( sw ) {
    sw->clear();
  }
}

//============================================================================
//
// class RuleEditWidget (the widget to edit one rule)
//
//============================================================================
RuleEditWidget::RuleEditWidget( KScoringManager *m, QWidget *p, const char *n )
  : QWidget( p ), dirty( false ), manager( m ), oldRuleName( QString() )
{
  kDebug(5100) <<"RuleEditWidget::RuleEditWidget()";

  setObjectName( n != 0 ? n : "RuleEditWidget" );

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setMargin( 5 );
  topLayout->setSpacing( KDialog::spacingHint() );

  //------------- Name, Servers, Groups ---------------------
  QGroupBox *groupB = new QGroupBox( i18n( "Properties" ), this );
  topLayout->addWidget( groupB );
  QGridLayout *groupL = new QGridLayout( groupB );
  groupL->setMargin( 8 );
  groupL->setSpacing( 5 );

  groupL->addItem( new QSpacerItem( 0, fontMetrics().lineSpacing() - 4 ), 0, 0 );

  // name
  ruleNameEdit = new KLineEdit( groupB );
  groupL->addWidget( ruleNameEdit, 1, 1 );
  QLabel *ruleNameLabel = new QLabel( i18nc( "@label rule name", "&Name:" ), groupB );
  ruleNameLabel->setBuddy( ruleNameEdit );
  ruleNameLabel->setObjectName( "ruleNameLabel" );
  groupL->addWidget( ruleNameLabel, 1, 0 );

  // groups
  groupsEdit = new KLineEdit( groupB );
  groupL->addWidget( groupsEdit, 2, 1 );
  QLabel *groupsLabel = new QLabel( i18n( "&Groups:" ), groupB );
  groupsLabel->setBuddy( groupsEdit );
  groupsLabel->setObjectName( "groupsLabel" );
  groupL->addWidget( groupsLabel, 2, 0 );

  QPushButton *groupsBtn = new QPushButton( i18n( "A&dd Group" ), groupB );
  connect( groupsBtn, SIGNAL(clicked()), SLOT(slotAddGroup()) );
  groupL->addWidget( groupsBtn, 3, 0 );

  groupsBox = new KComboBox( false, groupB );
  groupsBox->setDuplicatesEnabled( false );
  groupsBox->addItems( manager->getGroups() );
  groupsBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
  groupL->addWidget( groupsBox, 3, 1 );

  // expires
  expireCheck = new QCheckBox( i18n( "&Expire rule automatically" ), groupB );
  groupL->addWidget( expireCheck, 4, 0, 1, 2 );
  expireEdit = new KIntSpinBox( groupB );
  expireEdit->setRange( 1, 9999 );
  expireEdit->setValue( 30 );
  connect( expireEdit, SIGNAL(valueChanged(int)), SLOT(slotExpireEditChanged(int)) );
  groupL->addWidget( expireEdit, 5, 1 );
  expireLabel = new QLabel( i18n( "&Rule is valid for:" ), groupB );
  expireLabel->setBuddy( expireEdit );
  expireLabel->setObjectName( "expireLabel" );
  groupL->addWidget( expireLabel, 5, 0 );
  expireLabel->setEnabled( false );
  expireEdit->setEnabled( false );

  connect( expireCheck, SIGNAL(toggled(bool)), expireLabel, SLOT(setEnabled(bool)) );
  connect( expireCheck, SIGNAL(toggled(bool)), expireEdit, SLOT(setEnabled(bool)) );

  //------------- Conditions ---------------------
  QGroupBox *groupConds = new QGroupBox( i18n( "Conditions" ), this );
  topLayout->addWidget( groupConds );
  QGridLayout *condL = new QGridLayout( groupConds );
  condL->setMargin( 8 );
  condL->setSpacing( 5 );

  condL->addItem( new QSpacerItem( 0, fontMetrics().lineSpacing() - 4 ), 0, 0 );

  QButtonGroup *buttonGroup = new QButtonGroup( groupConds );

  linkModeAnd = new QRadioButton( i18n( "Match a&ll conditions" ), groupConds );
  buttonGroup->addButton( linkModeAnd );
  condL->addWidget( linkModeAnd, 1, 0 );
  linkModeOr = new QRadioButton( i18n( "Matc&h any condition" ), groupConds );
  buttonGroup->addButton( linkModeOr );
  condL->addWidget( linkModeOr, 1, 1 );
  linkModeAnd->setChecked( true );

  condEditor = new ConditionEditWidget( manager, groupConds );
  condL->addWidget( condEditor, 2, 0, 1, 2 );
  connect( condEditor, SIGNAL(widgetRemoved()), this, SLOT(slotShrink()) );

  //------------- Actions ---------------------
  QGroupBox *groupActions = new QGroupBox( i18n( "Actions" ), this );
  topLayout->addWidget( groupActions );
  QBoxLayout *actionL = new QVBoxLayout( groupActions );
  actionL->setMargin( 8 );
  actionL->setSpacing( 5 );
  actionL->addSpacing( fontMetrics().lineSpacing() - 4 );
  actionEditor = new ActionEditWidget( manager, groupActions );
  actionL->addWidget( actionEditor );
  connect( actionEditor, SIGNAL(widgetRemoved()), this, SLOT(slotShrink()) );

  topLayout->addStretch( 1 );

  kDebug(5100) <<"constructed RuleEditWidget";
}

RuleEditWidget::~RuleEditWidget()
{
}

void RuleEditWidget::slotEditRule( const QString &ruleName )
{
  KScoringRule *rule = manager->findRule( ruleName );
  if ( !rule ) {
    kDebug(5100) <<"no rule for ruleName" << ruleName;
    clearContents();
    return;
  }
  oldRuleName = rule->getName();
  ruleNameEdit->setText( rule->getName() );
  groupsEdit->setText( rule->getGroups().join( ";" ) );

  bool b = rule->getExpireDate().isValid();
  expireCheck->setChecked( b );
  expireEdit->setEnabled( b );
  expireLabel->setEnabled( b );
  if ( b ) {
    expireEdit->setValue( QDate::currentDate().daysTo( rule->getExpireDate() ) );
  } else {
    expireEdit->setValue( 30 );
  }
  if ( rule->getLinkMode() == KScoringRule::AND ) {
    linkModeAnd->setChecked( true );
  } else {
    linkModeOr->setChecked( true );
  }

  condEditor->slotEditRule( rule );
  actionEditor->slotEditRule( rule );

  kDebug(5100) <<"RuleEditWidget::slotEditRule() ready";
}

void RuleEditWidget::clearContents()
{
  ruleNameEdit->setText( "" );
  groupsEdit->setText( "" );
  expireCheck->setChecked( false );
  expireEdit->setValue( 30 );
  expireEdit->setEnabled( false );
  condEditor->slotEditRule( 0 );
  actionEditor->slotEditRule( 0 );
  oldRuleName.clear();
}

void RuleEditWidget::updateRule( KScoringRule *rule )
{
  oldRuleName.clear();
  QString groups = groupsEdit->text();
  if ( groups.isEmpty() ) {
    rule->setGroups( QStringList( ".*" ) );
  } else {
    rule->setGroups( groups.split( ";", QString::SkipEmptyParts ) );
  }
  bool b = expireCheck->isChecked();
  if ( b ) {
    rule->setExpireDate( QDate::currentDate().addDays( expireEdit->value() ) );
  } else {
    rule->setExpireDate( QDate() );
  }
  actionEditor->updateRule( rule );
  rule->setLinkMode( linkModeAnd->isChecked() ? KScoringRule::AND : KScoringRule::OR );
  condEditor->updateRule( rule );
  if ( rule->getName() != ruleNameEdit->text() ) {
    manager->setRuleName( rule, ruleNameEdit->text() );
  }
}

void RuleEditWidget::updateRule()
{
  KScoringRule *rule = manager->findRule( oldRuleName );
  if ( rule ) {
    updateRule( rule );
  }
}

void RuleEditWidget::slotAddGroup()
{
  QString grp = groupsBox->currentText();
  if ( grp.isEmpty() ) {
    return;
  }
  QString txt = groupsEdit->text().trimmed();
  if ( txt == ".*" || txt.isEmpty() ) {
    groupsEdit->setText( grp );
  } else {
    groupsEdit->setText( txt + ';' + grp );
  }
}

void RuleEditWidget::setDirty()
{
  kDebug(5100) <<"RuleEditWidget::setDirty()";
  if ( dirty ) {
    return;
  }
  dirty = true;
}

void RuleEditWidget::slotShrink()
{
  emit( shrink() );
}

void RuleEditWidget::slotExpireEditChanged( int value )
{
  expireEdit->setSuffix( i18np( " day", " days", value ) );
}

//============================================================================
//
// class RuleListWidget (the widget for managing a list of rules)
//
//============================================================================
RuleListWidget::RuleListWidget( KScoringManager *m, bool standalone, QWidget *p, const char *n )
  : QWidget( p ), alone( standalone ), manager( m )
{
  kDebug(5100) <<"RuleListWidget::RuleListWidget()";
  setObjectName( n != 0 ? n : "RuleListWidget" );
  QVBoxLayout *topL = new QVBoxLayout( this );
  topL->setMargin( standalone ? 0 : 5 );
  topL->setSpacing( KDialog::spacingHint() );

  ruleList = new K3ListBox( this );
  if ( standalone ) {
    connect( ruleList, SIGNAL(doubleClicked(Q3ListBoxItem*)),
             this, SLOT(slotEditRule(Q3ListBoxItem*)) );
    connect( ruleList, SIGNAL(returnPressed(Q3ListBoxItem*)),
             this, SLOT(slotEditRule(Q3ListBoxItem*)) );
  }
  connect( ruleList, SIGNAL(currentChanged(Q3ListBoxItem*)),
           this, SLOT(slotRuleSelected(Q3ListBoxItem*)) );
  topL->addWidget( ruleList );

  QHBoxLayout *btnL = new QHBoxLayout();
  btnL->setSpacing( KDialog::spacingHint() );

  topL->addItem( btnL );

  mRuleUp = new QPushButton( this );
  mRuleUp->setIcon( KIcon( "go-up" ) );
  mRuleUp->setToolTip( i18n( "Move rule up" ) );
  btnL->addWidget( mRuleUp );
  connect( mRuleUp, SIGNAL( clicked() ), SLOT( slotRuleUp() ) );
  mRuleDown = new QPushButton( this );
  mRuleDown->setIcon( KIcon( "go-down" ) );
  mRuleDown->setToolTip( i18n( "Move rule down" ) );
  btnL->addWidget( mRuleDown );
  connect( mRuleDown, SIGNAL( clicked() ), SLOT( slotRuleDown() ) );

  btnL = new QHBoxLayout();
  btnL->setSpacing( KDialog::spacingHint() );

  topL->addItem( btnL );

  editRule = 0;
  newRule = new QPushButton( this );
  newRule->setIcon( KIcon( "document-new" ) );
  newRule->setToolTip( i18n( "New rule" ) ),
  btnL->addWidget( newRule );
  connect( newRule, SIGNAL(clicked()), this, SLOT(slotNewRule()) );
  // if we're standalone, we need an additional edit button
  if ( standalone ) {
    editRule = new QPushButton( this );
    editRule->setIcon( KIcon( "edit" ) );
    editRule->setToolTip( i18n( "Edit rule" ) );
    btnL->addWidget( editRule );
    connect( editRule, SIGNAL(clicked()), this, SLOT(slotEditRule()) );
  }
  delRule = new QPushButton( this );
  delRule->setIcon( KIcon( "edit-delete" ) );
  delRule->setToolTip( i18n( "Remove rule" ) );
  btnL->addWidget( delRule );
  connect( delRule, SIGNAL(clicked()), this, SLOT(slotDelRule()) );
  copyRule = new QPushButton( this );
  copyRule->setIcon( KIcon( "edit-copy" ) );
  copyRule->setToolTip( i18n( "Copy rule" ) );
  btnL->addWidget( copyRule );
  connect( copyRule, SIGNAL(clicked()), this, SLOT(slotCopyRule()) );

  // the group filter
  QBoxLayout *filterL = new QVBoxLayout();
  topL->addItem( filterL );
  filterL->setSpacing( KDialog::spacingHint() );

  KComboBox *filterBox = new KComboBox( this );
  QStringList l = m->getGroups();
  filterBox->addItem( i18n( "<placeholder>all groups</placeholder>" ) );
  filterBox->addItems( l );
  filterBox->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
  connect( filterBox, SIGNAL(activated(const QString&)),
           this, SLOT(slotGroupFilter(const QString&)) );
  slotGroupFilter( i18n( "<placeholder>all groups</placeholder>" ) );
  QLabel *lab = new QLabel( i18n( "Sho&w only rules for group:" ), this );
  lab->setBuddy( filterBox );

  filterL->addWidget( lab );
  filterL->addWidget( filterBox );

  connect( manager, SIGNAL(changedRules()), this, SLOT(updateRuleList()) );
  connect( manager, SIGNAL(changedRuleName(const QString&,const QString&)),
           this, SLOT(slotRuleNameChanged(const QString&,const QString&)) );

  updateRuleList();
  updateButton();
}

RuleListWidget::~RuleListWidget()
{
}

void RuleListWidget::updateButton()
{
  bool state = ruleList->count() > 0;
  if( editRule ) {
    editRule->setEnabled( state );
  }
  delRule->setEnabled( state );
  copyRule->setEnabled( state );

  Q3ListBoxItem *item = ruleList->item( ruleList->currentItem() );
  if ( item ) {
    mRuleUp->setEnabled( item->prev() != 0 );
    mRuleDown->setEnabled( item->next() != 0 );
  }
}

void RuleListWidget::updateRuleList()
{
  emit leavingRule();
  kDebug(5100) <<"RuleListWidget::updateRuleList()";
  QString curr = ruleList->currentText();
  ruleList->clear();
  if ( group == i18n( "<placeholder>all groups</placeholder>" ) ) {
    QStringList l = manager->getRuleNames();
    ruleList->insertStringList( l );
  } else {
    KScoringManager::ScoringRuleList l = manager->getAllRules();
    for ( KScoringRule *rule = l.first(); rule; rule = l.next() ) {
      if ( rule->matchGroup( group ) ) {
        ruleList->insertItem( rule->getName() );
      }
    }
  }
  int index = setCurrentItem( ruleList, curr );
  if ( index < 0 ) {
    ruleList->setCurrentItem( 0 );
    slotRuleSelected( ruleList->currentText() );
  } else {
    slotRuleSelected( curr );
  }
}

void RuleListWidget::updateRuleList( const KScoringRule *rule )
{
  kDebug(5100) <<"RuleListWidget::updateRuleList(" << rule->getName() <<")";
  QString name = rule->getName();
  updateRuleList();
  slotRuleSelected(name);
}

void RuleListWidget::slotRuleNameChanged( const QString &oldName, const QString &newName )
{
  int ind = ruleList->currentItem();
  for ( uint i=0; i<ruleList->count(); ++i ) {
    if ( ruleList->text(i) == oldName ) {
      ruleList->changeItem( newName, i );
      ruleList->setCurrentItem( ind );
      return;
    }
  }
}

void RuleListWidget::slotEditRule( const QString &s )
{
  emit ruleEdited( s );
}

void RuleListWidget::slotEditRule()
{
  if ( ruleList->currentItem() >= 0 ) {
    emit ruleEdited( ruleList->currentText() );
  } else if ( ruleList->count() == 0 ) {
    emit ruleEdited( QString() );
  }
}

void RuleListWidget::slotEditRule( Q3ListBoxItem *item )
{
  slotEditRule( item->text() );
}

void RuleListWidget::slotGroupFilter( const QString &s )
{
  group = s;
  updateRuleList();
}

void RuleListWidget::slotRuleSelected( const QString &ruleName )
{
  emit leavingRule();
  kDebug(5100) <<"RuleListWidget::slotRuleSelected(" << ruleName <<")";
  if ( ruleName != ruleList->currentText() ) {
    setCurrentItem( ruleList, ruleName );
  }
  updateButton();
  emit ruleSelected( ruleName );
}

void RuleListWidget::slotRuleSelected( Q3ListBoxItem *item )
{
  if ( !item ) {
    return;
  }
  QString ruleName = item->text();
  slotRuleSelected( ruleName );
}

void RuleListWidget::slotRuleSelected( int index )
{
  uint idx = index;
  if ( idx >= ruleList->count() ) {
    return;
  }
  QString ruleName = ruleList->text( index );
  slotRuleSelected( ruleName );
}

void RuleListWidget::slotNewRule()
{
  emit leavingRule();
  KScoringRule *rule = manager->addRule();
  updateRuleList( rule );
  if ( alone ) {
    slotEditRule( rule->getName() );
  }
  updateButton();
}

void RuleListWidget::slotDelRule()
{
  KScoringRule *rule = manager->findRule( ruleList->currentText() );
  if ( rule ) {
    manager->deleteRule( rule );
  }
  // goto the next rule
  if ( !alone ) {
    slotEditRule();
  }
  updateButton();
}

void RuleListWidget::slotCopyRule()
{
  emit leavingRule();
  QString ruleName = ruleList->currentText();
  KScoringRule *rule = manager->findRule( ruleName );
  if ( rule ) {
    KScoringRule *nrule = manager->copyRule( rule );
    updateRuleList( nrule );
    slotEditRule( nrule->getName() );
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
    if ( item ) {
      below = manager->findRule( item->text() );
    }
  }
  if ( rule && below ) {
    manager->moveRuleAbove( rule, below );
  }
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
    if ( item ) {
      above = manager->findRule( item->text() );
    }
  }
  if ( rule && above ) {
    manager->moveRuleBelow( rule, above );
  }
  updateRuleList();
  updateButton();
}

//============================================================================
//
// class KScoringEditor (the score edit dialog)
//
//============================================================================
KScoringEditor *KScoringEditor::scoreEditor = 0;

KScoringEditor::KScoringEditor( KScoringManager *m, QWidget *parent )
  : KDialog( parent ), manager( m )
{
  setCaption( i18n( "Rule Editor" ) );
  setButtons( Ok|Apply|Cancel );
  setDefaultButton( Ok );
  setModal( false );
  showButtonSeparator( true );
  manager->pushRuleList();
  if ( !scoreEditor ) {
    scoreEditor = this;
  }
  kDebug(5100) <<"KScoringEditor::KScoringEditor()";
  // the left side gives an overview about all rules, the right side
  // shows a detailed view of an selected rule
  QWidget *w = new QWidget( this );
  setMainWidget( w );
  QHBoxLayout *hbl = new QHBoxLayout( w );
  hbl->setMargin( 0 );
  hbl->setSpacing( spacingHint() );

  ruleLister = new RuleListWidget( manager, false, w );
  hbl->addWidget( ruleLister );
  ruleEditor = new RuleEditWidget( manager, w );
  hbl->addWidget( ruleEditor );
  connect( ruleLister, SIGNAL(ruleSelected(const QString&)),
           ruleEditor, SLOT(slotEditRule(const QString&)) );
  connect( ruleLister, SIGNAL(leavingRule()), ruleEditor, SLOT(updateRule()) );
  connect( ruleEditor, SIGNAL(shrink()), SLOT(slotShrink()) );
  connect( this, SIGNAL(finished()), SLOT(slotFinished()) );
  connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
  connect( this, SIGNAL(cancelClicked()), SLOT(slotCancel()) );
  connect( this, SIGNAL(applyClicked()), SLOT(slotApply()) );
  ruleLister->slotRuleSelected( 0 );
  resize( 550, sizeHint().height() );
}

void KScoringEditor::setDirty()
{
  enableButton( Apply, true );
}

KScoringEditor::~KScoringEditor()
{
  scoreEditor = 0;
}

KScoringEditor *KScoringEditor::createEditor( KScoringManager *m, QWidget *parent )
{
  if ( scoreEditor ) {
    return scoreEditor;
  } else {
    return new KScoringEditor( m, parent );
  }
}

void KScoringEditor::setRule( KScoringRule *r )
{
  kDebug(5100) <<"KScoringEditor::setRule(" << r->getName() <<")";
  QString ruleName = r->getName();
  ruleLister->slotRuleSelected( ruleName );
}

void KScoringEditor::slotShrink()
{
  QTimer::singleShot( 5, this, SLOT(slotDoShrink()) );
}

void KScoringEditor::slotDoShrink()
{
  updateGeometry();
  QApplication::sendPostedEvents();
  resize( width(), sizeHint().height() );
}

void KScoringEditor::slotApply()
{
  QString ruleName = ruleLister->currentRule();
  KScoringRule *rule = manager->findRule( ruleName );
  if ( rule ) {
    ruleEditor->updateRule( rule );
    ruleLister->updateRuleList( rule );
  }
  manager->removeTOS();
  manager->pushRuleList();
}

void KScoringEditor::slotOk()
{
  slotApply();
  manager->removeTOS();
  KDialog::accept();
  manager->editorReady();
}

void KScoringEditor::slotCancel()
{
  manager->popRuleList();
  KDialog::reject();
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
KScoringEditorWidgetDialog::KScoringEditorWidgetDialog( KScoringManager *m,
                                                        const QString &r,
                                                        QWidget *p )
  : KDialog( p ), manager( m ), ruleName( r )
{
  setCaption( i18n( "Edit Rule" ) );
  setButtons( KDialog::Ok|KDialog::Apply|KDialog::Close );
  setDefaultButton( KDialog::Ok );
  setModal( true );
  showButtonSeparator( true );
  QFrame *f = new QFrame( this );
  setMainWidget( f );
  QBoxLayout *topL = new QVBoxLayout( f );
  ruleEditor = new RuleEditWidget( manager, f );
  connect( ruleEditor, SIGNAL(shrink()), SLOT(slotShrink()) );
  connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
  topL->addWidget( ruleEditor );
  ruleEditor->slotEditRule( ruleName );
  resize( 0, 0 );
}

void KScoringEditorWidgetDialog::slotApply()
{
  KScoringRule *rule = manager->findRule( ruleName );
  if ( rule ) {
    ruleEditor->updateRule( rule );
    ruleName = rule->getName();
  }
}

void KScoringEditorWidgetDialog::slotOk()
{
  slotApply();
  KDialog::accept();
}

void KScoringEditorWidgetDialog::slotShrink()
{
  QTimer::singleShot( 5, this, SLOT(slotDoShrink()) );
}

void KScoringEditorWidgetDialog::slotDoShrink()
{
  updateGeometry();
  QApplication::sendPostedEvents();
  resize( width(), sizeHint().height() );
}

//============================================================================
//
// class KScoringEditorWidget (a reusable widget for config dialog...)
//
//============================================================================
KScoringEditorWidget::KScoringEditorWidget( KScoringManager *m, QWidget *p, const char *n )
  : QWidget( p ), manager( m )
{
  setObjectName( n );
  QBoxLayout *topL = new QVBoxLayout( this );
  ruleLister = new RuleListWidget( manager, true, this );
  topL->addWidget( ruleLister );
  connect( ruleLister, SIGNAL(ruleEdited(const QString&)),
           this, SLOT(slotRuleEdited(const QString &)) );
}

KScoringEditorWidget::~KScoringEditorWidget()
{
  manager->editorReady();
}

void KScoringEditorWidget::slotRuleEdited( const QString &ruleName )
{
  KScoringEditorWidgetDialog dlg( manager, ruleName, this );
  dlg.exec();
  ruleLister->updateRuleList();
}

#include "kscoringeditor.moc"
