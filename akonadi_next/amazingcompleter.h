
#ifndef AMAZING_COMPLETER_H
#define AMAZING_COMPLETER_H

#include <QObject>
#include <QWidget>
#include <QAbstractItemModel>

#include "akonadi_next_export.h"

class QAbstractItemView;

class AmazingCompleterPrivate;

class AKONADI_NEXT_EXPORT AmazingCompleter : public QObject
{
  Q_OBJECT
public:

  enum ViewHandler { Popup, NoPopup };

  AmazingCompleter(/* QAbstractItemModel *model, */ QObject *parent = 0);

  void setWidget(QWidget *widget);

  void setView(QAbstractItemView *view, ViewHandler = Popup);

  void setModel(QAbstractItemModel *model);

  /**
  The data from this is put into the lineedit
  */
  void setCompletionRole(int role);

  /**
  This role is passed to match().
  */
  void setMatchingRole(int role);

public slots:
  void setCompletionPrefixString(const QString &matchData);
  void setCompletionPrefix(const QVariant &matchData);

  void sourceRowsInserted(const QModelIndex &parent, int start, int end);

protected:
  virtual void connectModelToView(QAbstractItemModel *model, QAbstractItemView *view);

private:
  Q_DECLARE_PRIVATE(AmazingCompleter);

  AmazingCompleterPrivate *d_ptr;

};

#endif
