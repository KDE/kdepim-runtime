/*
 * This file is part of the Akonadi Mail example.
 *
 * Copyright 2009  Stephen Kelly <steveire@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

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



