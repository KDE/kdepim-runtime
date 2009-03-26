
#include "mailcomposer.h"

#include <QGridLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>

#include "emaillineedit.h"

MailComposer::MailComposer(Akonadi::Session *session, QWidget *parent)
{
  QGridLayout *layout = new QGridLayout();

  QLabel *toLabel = new QLabel(this);
  toLabel->setText("To:");
  EmailLineEdit *emailLineEdit = new EmailLineEdit(session, this);

  QLabel *subjectLabel = new QLabel(this);
  subjectLabel->setText("Subject:");
  QLineEdit *subjectLineEdit = new QLineEdit(this);

  QTextEdit *textEdit = new QTextEdit(this);

  layout->addWidget(toLabel, 0, 0);
  layout->addWidget(emailLineEdit, 0, 1);
  layout->addWidget(subjectLabel, 1, 0);
  layout->addWidget(subjectLineEdit, 1, 1);
  layout->addWidget(textEdit, 2, 0, 1, 2);

  this->setLayout(layout);
}



