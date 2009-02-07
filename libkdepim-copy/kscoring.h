/*
  kscoring.h

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

#ifndef KDEPIM_KSCORING_H
#define KDEPIM_KSCORING_H

#include "kdepim_export.h"

#include <KDialog>

#include <QColor>
#include <QMap>
#include <QObject>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <Q3Dict>
#include <Q3PtrList>
#include <Q3PtrStack>

#include <unistd.h>

class QDomNode;
class QDomDocument;
class QDomElement;
class QLabel;

namespace KPIM {

/**
  The following classes ScorableArticle, ScorableGroup define
  the interface for the scoring. Any application using this mechanism should
  create its own subclasses of this classes. The scoring itself will be handled
  by the ScoringManager class.
 */

//----------------------------------------------------------------------------
class KDEPIM_EXPORT ScorableGroup
{
public:
  virtual ~ScorableGroup();
};

class KDEPIM_EXPORT ScorableArticle
{
  public:
    virtual ~ScorableArticle();

    virtual void addScore( short ) {}
    virtual void displayMessage( const QString & );
    virtual void changeColor( const QColor & ) {}
    virtual void markAsRead() {}
    virtual QString from() const = 0;
    virtual QString subject() const = 0;
    virtual QString getHeaderByType( const QString & ) const = 0;
};

//----------------------------------------------------------------------------
/**
  Base class for other Action classes.
 */
class KDEPIM_EXPORT ActionBase {
  public:
    ActionBase();
    virtual ~ActionBase();
    virtual QString toString() const = 0;
    virtual void apply( ScorableArticle & ) const = 0;
    virtual ActionBase *clone() const = 0;
    virtual int getType() const = 0;
    virtual QString getValueString() const { return QString(); }
    virtual void setValue( const QString & ) {}
    static ActionBase *factory( int type, const QString &value );
    static QStringList userNames();
    static QString userName( int type );
    static int getTypeForName( const QString &name );
    static int getTypeForUserName( const QString &name );
    QString userName() { return userName( getType() ); }
    enum ActionTypes {
      SETSCORE,
      NOTIFY,
      COLOR,
      MARKASREAD
    };
};

class KDEPIM_EXPORT ActionColor : public ActionBase {
  public:
    ActionColor( const QColor & );
    ActionColor( const QString & );
    ActionColor( const ActionColor & );
    virtual ~ActionColor();
    virtual QString toString() const;
    virtual int getType() const { return COLOR; }
    virtual QString getValueString() const { return color.name(); }
    virtual void setValue( const QString &s ) { color.setNamedColor(s); }
    void setValue( const QColor &c ) { color = c; }
    QColor value() const { return color; }
    virtual void apply( ScorableArticle & ) const;
    virtual ActionColor *clone() const;

  private:
    QColor color;
};

class KDEPIM_EXPORT ActionSetScore : public ActionBase {
  public:
    ActionSetScore( short );
    ActionSetScore( const ActionSetScore & );
    ActionSetScore( const QString & );
    virtual ~ActionSetScore();
    virtual QString toString() const;
    virtual int getType() const { return SETSCORE; }
    virtual QString getValueString() const { return QString::number(val); }
    virtual void setValue( const QString &s ) { val = s.toShort(); }
    void setValue( short v ) { val = v; }
    short value() const { return val; }
    virtual void apply( ScorableArticle & ) const;
    virtual ActionSetScore *clone() const;

  private:
    short val;
};

class KDEPIM_EXPORT ActionNotify : public ActionBase {
  public:
    ActionNotify( const QString & );
    ActionNotify( const ActionNotify & );
    virtual ~ActionNotify() {}
    virtual QString toString() const;
    virtual int getType() const { return NOTIFY; }
    virtual QString getValueString() const { return note; }
    virtual void setValue( const QString &s ) { note = s; }
    virtual void apply( ScorableArticle & ) const;
    virtual ActionNotify *clone() const;

  private:
    QString note;
};

class KDEPIM_EXPORT ActionMarkAsRead : public ActionBase {
    public:
    ActionMarkAsRead();
    ActionMarkAsRead( const ActionMarkAsRead & );
    virtual ~ActionMarkAsRead() {}
    virtual QString toString() const;
    virtual int getType() const { return MARKASREAD; }
    virtual void apply( ScorableArticle &article ) const;
    virtual ActionMarkAsRead *clone() const;
};

class KDEPIM_EXPORT NotifyCollection
{
  public:
    NotifyCollection();
    ~NotifyCollection();
    void addNote( const ScorableArticle &, const QString & );
    QString collection() const;
    void displayCollection( QWidget *p=0 ) const;
  private:
    struct article_info {
      QString from;
      QString subject;
    };
    typedef QList<article_info> article_list;
    typedef Q3Dict<article_list> note_list;
    note_list notifyList;
};

//----------------------------------------------------------------------------
class KDEPIM_EXPORT KScoringExpression
{
  friend class KScoringRule;
  public:
    enum Condition {
      CONTAINS,
      MATCH,
      EQUALS,
      SMALLER,
      GREATER,
      MATCHCS
    };

    KScoringExpression( const QString &, const QString &, const QString &, const QString & );
    ~KScoringExpression();

    bool match( ScorableArticle &a ) const ;
    QString getTypeString() const;
    static QString getTypeString( int );
    int getType() const;
    QString toString() const;
    void write( QTextStream & ) const;

    bool isNeg() const { return neg; }
    Condition getCondition() const { return cond; }
    QString getExpression() const { return expr_str; }
    QString getHeader() const { return header; }
    static QStringList conditionNames();
    static QStringList headerNames();
    static int getConditionForName( const QString & );
    static QString getNameForCondition( int );

  private:
    bool neg;
    QString header;
    const char *c_header;
    Condition cond;
    QRegExp expr;
    QString expr_str;
    int expr_int;
};

//----------------------------------------------------------------------------
class KDEPIM_EXPORT KScoringRule
{
  friend class KScoringManager;
  public:
    KScoringRule( const QString &name );
    KScoringRule( const KScoringRule &r );
    ~KScoringRule();

    typedef Q3PtrList<KScoringExpression> ScoreExprList;
    typedef Q3PtrList<ActionBase> ActionList;
    typedef QStringList GroupList;
    enum LinkMode {
      AND,
      OR
    };

    QString getName() const { return name; }
    QStringList getGroups() const { return groups; }
    void setGroups( const QStringList &l ) { groups = l; }
    LinkMode getLinkMode() const { return link; }
    QString getLinkModeName() const;
    QString getExpireDateString() const;
    QDate getExpireDate() const { return expires; }
    void setExpireDate( const QDate &d ) { expires = d; }
    bool isExpired() const;
    ScoreExprList getExpressions() const { return expressions; }
    ActionList getActions() const { return actions; }
    void cleanExpressions();
    void cleanActions();

    bool matchGroup( const QString &group ) const ;
    void applyRule( ScorableArticle &a ) const;
    void applyRule( ScorableArticle &a, const QString &group ) const;
    void applyAction( ScorableArticle &a ) const;

    void setLinkMode( const QString &link );
    void setLinkMode( LinkMode m ) { link = m; }
    void setExpire( const QString &exp );
    void addExpression( KScoringExpression * );
    void addGroup( const QString &group ) { groups.append(group); }
    void addAction( int, const QString & );
    void addAction(ActionBase*);

    void updateXML( QDomElement &e, QDomDocument &d );
    QString toString() const;

    // writes the rule in XML format into the textstream
    void write( QTextStream & ) const;

  protected:
    //! assert that the name is unique
    void setName( const QString &n ) { name = n; }

  private:
    QString name;
    GroupList groups;
    //ServerList servers;
    LinkMode link;
    ScoreExprList expressions;
    ActionList actions;
    QDate expires;
};

/** this helper class implements a stack for lists of lists of rules.
    With the help of this class it is very easy for the KScoringManager
    to temporary drop lists of rules and restore them afterwards
*/
class KDEPIM_EXPORT RuleStack
{
  public:
    RuleStack();
    ~RuleStack();
    //! puts the list on the stack, doesn't change the list
    void push( Q3PtrList<KScoringRule>& );
    //! clears the argument list and copy the content of the TOS into it
    //! after that the TOS gets dropped
    void pop( Q3PtrList<KScoringRule>& );
    //! like pop but without dropping the TOS
    void top( Q3PtrList<KScoringRule>& );
    //! drops the TOS
    void drop();

  private:
    Q3PtrStack< Q3PtrList<KScoringRule> > stack;
};

//----------------------------------------------------------------------------
// Manages the score rules.
class KDEPIM_EXPORT KScoringManager : public QObject
{
  Q_OBJECT
  public:
    // this is the container for all rules
    typedef Q3PtrList<KScoringRule> ScoringRuleList;

    KScoringManager( const QString &appName = QString() );
    virtual ~KScoringManager();

    // returns a list of all available groups, must be overridden
    virtual QStringList getGroups() const = 0;

    //! returns a list of common (or available) headers
    //! defaults to returning { Subject, From, Message-ID, Date }
    virtual QStringList getDefaultHeaders() const;

    // setting current server and group and calling applyRules(ScorableArticle&)
    void applyRules( ScorableArticle &article, const QString &group );
    // assuming a properly set group
    void applyRules( ScorableArticle & );
    // same as above
    void applyRules( ScorableGroup *group );

    // pushes the current rule list onto a stack
    void pushRuleList();
    // restores the current rule list from list stored on a stack
    // by a previous call to pushRuleList (this implicitly deletes the
    // current rule list)
    void popRuleList();
    // removes the TOS from the stack of rule lists
    void removeTOS();

    KScoringRule *addRule( KScoringRule * );
    KScoringRule *addRule( const ScorableArticle &a, const QString &group, short score=0 );
    KScoringRule *addRule();
    void cancelNewRule( KScoringRule * );
    void deleteRule( KScoringRule * );
    void editRule( KScoringRule *e, QWidget *w=0 );
    KScoringRule *copyRule( KScoringRule * );
    void moveRuleAbove( KScoringRule *above, KScoringRule *below );
    void moveRuleBelow( KScoringRule *below, KScoringRule *above );
    void setGroup( const QString &g );
    // has to be called after setGroup() or initCache()
    bool hasRulesForCurrentGroup();
    QString findUniqueName() const;

    /** called from an editor whenever it finishes editing the rule base,
        causes the finishedEditing signal to be emitted */
    void editorReady();

    ScoringRuleList getAllRules() const { return allRules; }
    KScoringRule *findRule( const QString & );
    QStringList getRuleNames();
    void setRuleName( KScoringRule *, const QString & );
    int getRuleCount() const { return allRules.count(); }
    QString toString() const;

    bool setCacheValid( bool v );
    bool isCacheValid() { return cacheValid; }
    void initCache( const QString &group );

    void load();
    void save();

    //--------------- Properties
    virtual bool canScores() const { return true; }
    virtual bool canNotes() const { return true; }
    virtual bool canColors() const { return false; }
    virtual bool canMarkAsRead() const { return false; }
    virtual bool hasFeature( int );

  Q_SIGNALS:
    void changedRules();
    void changedRuleName( const QString &oldName, const QString &newName );
    void finishedEditing();

  private:
    void addRuleInternal( KScoringRule *e );
    void expireRules();

    QDomDocument createXMLfromInternal();
    void createInternalFromXML(QDomNode);

    // list of all Rules
    ScoringRuleList allRules;

    // the stack for temporary storing rule lists
    RuleStack stack;

    // for the cache
    bool cacheValid;
    // current rule set, ie the cache
    ScoringRuleList ruleList;
    //QString server;
    QString group;

    //ScorableServer* _s;

    // filename of the scorefile
    QString mFilename;
};

//----------------------------------------------------------------------------
class KDEPIM_EXPORT NotifyDialog : public KDialog
{
  Q_OBJECT
  public:
    static void display( ScorableArticle &, const QString & );

  protected Q_SLOTS:
    void slotShowAgainToggled( bool );

  private:
    NotifyDialog( QWidget *p=0 );
    static NotifyDialog *me;

    QLabel *note;
    QString msg;
    typedef QMap<QString,bool> NotesMap;
    static NotesMap dict;
};

}

#endif
