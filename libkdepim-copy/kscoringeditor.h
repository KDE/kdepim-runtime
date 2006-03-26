/*
    kscoringeditor.h

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

#ifndef SCOREEDITWIDGET_H
#define SCOREEDITWIDGET_H

#include <qmap.h>
//Added by qt3to4:
#include <QLabel>
#include <kdialogbase.h>
#include <q3table.h>
#include <QFrame>
#include <QStackedWidget>
#include <klistbox.h>
#include "kwidgetlister.h"

#include <kdepimmacros.h>

class KComboBox;
class KLineEdit;
class KIntSpinBox;
class QLabel;
class Q3ListBoxItem;
class QPushButton;
class QCheckBox;
class QRadioButton;

class KScoringRule;
class KScoringExpression;
class KScoringManager;
class ActionBase;
class KScoringEditor;
class ScoreEditWidget;
class KColorCombo;

/** this widget implements an editor for one condition.
    It is used in ExpressionEditWidget
*/
class KDE_EXPORT SingleConditionWidget : public QFrame
{
  Q_OBJECT
  friend class ConditionEditWidget;
public:
  SingleConditionWidget(KScoringManager *,QWidget *p =0, const char *n =0);
  ~SingleConditionWidget();
  void setCondition(KScoringExpression*);
  KScoringExpression *createCondition() const;
  void clear();

protected slots:
  void toggleRegExpButton( int );
  void showRegExpDialog();

private:
  /** marks a condition as negated */
  QCheckBox *neg;
  /** list of possible headers */
  KComboBox *headers;
  /** list of match types */
  KComboBox *matches;
  /** the expression which will be matched */
  KLineEdit *expr;
  /** the button to open the regexp-editor */
  QPushButton *regExpButton;

  KScoringManager *manager;
};

/** this widget implements the conditions editor
 */
class ConditionEditWidget: public KWidgetLister
{
  Q_OBJECT
public:
  ConditionEditWidget(KScoringManager *,QWidget *p =0, const char *n =0);
  ~ConditionEditWidget();
  QWidget* createWidget(QWidget*);
  void updateRule(KScoringRule*);
  void clearWidget(QWidget*);
public slots:
  void slotEditRule(KScoringRule*);
private:
  KScoringManager *manager;
};

/** this widget implements an editor for one action.
    It is used in ActionEditWidget
*/
class SingleActionWidget : public QWidget
{
  Q_OBJECT
  friend class ActionEditWidget;
public:
  SingleActionWidget(KScoringManager *m,QWidget *p =0, const char *n =0);
  ~SingleActionWidget();
  void setAction(ActionBase*);
  ActionBase *createAction() const;
  void clear();
private:
  /** the list of available action */
  KComboBox *types;
  /** the stack of the edit widgets for each action type */
  QStackedWidget *stack;
  /** the notify action editor */
  KLineEdit *notifyEditor;
  /** the score acton editor */
  KIntSpinBox *scoreEditor;
  /** the color action editor */
  KColorCombo *colorEditor;
  /** the dummy label */
  QLabel *dummyLabel;

  KScoringManager *manager;
};

/** this widget implements the action editor
 */
class KDE_EXPORT ActionEditWidget : public KWidgetLister
{
  Q_OBJECT
public:
  ActionEditWidget(KScoringManager *m,QWidget *p =0, const char *n =0);
  ~ActionEditWidget();
  QWidget* createWidget(QWidget *parent);
  void updateRule(KScoringRule*);
  void clearWidget(QWidget *);
public slots:
  void slotEditRule(KScoringRule *);
private:
  KScoringManager *manager;
};

/** This widget implements the rule editor
 */
class RuleEditWidget : public QWidget
{
  Q_OBJECT
public:
  RuleEditWidget(KScoringManager *m,QWidget *p =0, const char *n =0);
  ~RuleEditWidget();
public slots:
  void setDirty();
  void slotEditRule(const QString&);
  void updateRule(KScoringRule*);
  void updateRule();
signals:
  void shrink();
protected slots:
  void slotAddGroup();
  void slotShrink();
private slots:
  void slotExpireEditChanged(int value);
private:
  void clearContents();

  bool dirty;
  /** the name of the rule */
  KLineEdit *ruleNameEdit;
  /** the list of groups this rule applies to */
  KLineEdit *groupsEdit;
  /** list of all available groups */
  KComboBox *groupsBox;
  /** the expire enable */
  QCheckBox *expireCheck;
  /** the label to the expireCheck */
  QLabel *expireLabel;
  /** the expire delay */
  KIntSpinBox *expireEdit;
  /** the link modes of the conditions */
  QRadioButton *linkModeOr, *linkModeAnd;
  /** the actions editor */
  ActionEditWidget *actionEditor;
  /** the conditions editor */
  ConditionEditWidget *condEditor;

  KScoringManager *manager;

  // the old name of the current rule
  QString oldRuleName;
};

/** This widget shows a list of rules with buttons for
    copy, delete aso.
*/
class KDE_EXPORT RuleListWidget : public QWidget
{
  Q_OBJECT
public:
  RuleListWidget(KScoringManager *m, bool =false, QWidget *p =0, const char *n =0);
  ~RuleListWidget();
  QString currentRule() const { return ruleList->currentText(); }
protected:
  void updateButton();

signals:
  void ruleSelected(const QString&);
  void ruleEdited(const QString&);
  void leavingRule();
public slots:
  void slotRuleSelected(const QString&);
  void slotRuleSelected(Q3ListBoxItem *);
  void slotRuleSelected(int);
  void updateRuleList();
  void updateRuleList(const KScoringRule*);
  void slotRuleNameChanged(const QString&,const QString&);
protected slots:
  void slotGroupFilter(const QString&);
  void slotEditRule(Q3ListBoxItem*);
  void slotEditRule(const QString&);
  void slotEditRule();
  void slotDelRule();
  void slotNewRule();
  void slotCopyRule();
  void slotRuleUp();
  void slotRuleDown();

private:
  /** the list of rules */
  KListBox *ruleList;
  /** the current group */
  QString group;
  /** marks if we're alone or together with the edit widget */
  bool alone;

  KScoringManager *manager;

  QPushButton *editRule;
  QPushButton *newRule;
  QPushButton *delRule;
  QPushButton *copyRule;
  QPushButton *mRuleUp;
  QPushButton *mRuleDown;
};

class KDE_EXPORT KScoringEditor : public KDialogBase
{
  Q_OBJECT
public:
  ~KScoringEditor();
  void setRule(KScoringRule*);
  static KScoringEditor *createEditor(KScoringManager* m, QWidget *parent=0, const char *name=0);
  static KScoringEditor *editor() { return scoreEditor; }
  void setDirty();
protected:
  KScoringEditor(KScoringManager* m, QWidget *parent=0, const char *name=0);
private:
  /** the editor for the current rule */
  RuleEditWidget* ruleEditor;
  /** the list of known rules */
  RuleListWidget *ruleLister;
protected slots:
  void slotShrink();
  void slotDoShrink();
  void slotApply();
  void slotOk();
  void slotCancel();
  void slotFinished();
private:
  KScoringManager *manager;
  ScoreEditWidget *edit;
  /** make sure that there is only one instance of ourselve */
  static KScoringEditor *scoreEditor;
};

class KScoringEditorWidgetDialog : public KDialogBase
{
  Q_OBJECT
public:
  KScoringEditorWidgetDialog(KScoringManager *m, const QString& rName, QWidget *parent=0, const char *name=0);
protected slots:
  void slotApply();
  void slotOk();
  void slotShrink();
  void slotDoShrink();
private:
  RuleEditWidget *ruleEditor;
  KScoringManager *manager;
  QString ruleName;
};

class KDE_EXPORT KScoringEditorWidget : public QWidget
{
  Q_OBJECT
public:
  KScoringEditorWidget(KScoringManager *m,QWidget *p =0, const char *n =0);
  ~KScoringEditorWidget();
protected slots:
  void slotRuleEdited(const QString&);
private:
  RuleListWidget *ruleLister;
  KScoringManager *manager;
};


#endif // SCOREEDITWIDGET_H
