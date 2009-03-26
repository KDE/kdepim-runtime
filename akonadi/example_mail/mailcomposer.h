
#ifndef MAILCOMPOSER_H
#define MAILCOMPOSER_H

#include <QWidget>

#include <akonadi/session.h>

class MailComposer : public QWidget
{
  Q_OBJECT
public:
  MailComposer(Akonadi::Session *session, QWidget *parent = 0);

};

#endif

