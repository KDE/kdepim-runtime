/* This file is part of the KDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// Test program for libkdepim/email.h
#include "email.h"

#include <kcmdlineargs.h>
#include <kapplication.h>
#include <kdebug.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static bool check(const QString& txt, const QString& a, const QString& b)
{
  if (a == b) {
    kdDebug() << txt << " : checking '" << a << "' against expected value '" << b << "'... " << "ok" << endl;
  }
  else {
    kdDebug() << txt << " : checking '" << a << "' against expected value '" << b << "'... " << "KO !" << endl;
    exit(1);
  }
  return true;
}

static bool checkGetNameAndEmail(const QString& input, const QString& expName, const QString& expEmail, bool expRetVal)
{
  QString name, email;
  bool retVal = KPIM::getNameAndMail(input, name, email);
  check( "getNameAndMail " + input + " retVal", retVal?"true":"false", expRetVal?"true":"false" );
  check( "getNameAndMail " + input + " name", name, expName );
  check( "getNameAndMail " + input + " email", email, expEmail );
  return true;
}

int main(int argc, char *argv[])
{
  KApplication::disableAutoDcopRegistration();
  KCmdLineArgs::init( argc, argv, "testemail", 0, 0, 0, 0 );
  KApplication app( false, false );

  // Empty input
  checkGetNameAndEmail( QString::null, QString::null, QString::null, false );

  // Email only
  checkGetNameAndEmail( "faure@kde.org", QString::null, "faure@kde.org", false );

  // Normal case
  checkGetNameAndEmail( "David Faure <faure@kde.org>", "David Faure", "faure@kde.org", true );

  // Double-quotes
  checkGetNameAndEmail( "\"Faure, David\" <faure@kde.org>", "Faure, David", "faure@kde.org", true );
  checkGetNameAndEmail( "<faure@kde.org> \"David Faure\"", "David Faure", "faure@kde.org", true );

  // Parenthesis
  checkGetNameAndEmail( "faure@kde.org (David Faure)", "David Faure", "faure@kde.org", true );
  checkGetNameAndEmail( "(David Faure) faure@kde.org", "David Faure", "faure@kde.org", true );

  // Double-quotes inside parenthesis
  checkGetNameAndEmail( "faure@kde.org (David \"Crazy\" Faure)", "David \"Crazy\" Faure", "faure@kde.org", true );
  checkGetNameAndEmail( "(David \"Crazy\" Faure) faure@kde.org", "David \"Crazy\" Faure", "faure@kde.org", true );

  // Parenthesis inside double-quotes
  checkGetNameAndEmail( "\"Faure (David)\" <faure@kde.org>", "Faure (David)", "faure@kde.org", true );
  checkGetNameAndEmail( "<faure@kde.org> \"Faure (David)\"", "Faure (David)", "faure@kde.org", true );

  // Space in email
  checkGetNameAndEmail( "David Faure < faure@kde.org >", "David Faure", "faure@kde.org", true );

  // Check that '@' in name doesn't confuse it
  checkGetNameAndEmail( "faure@kde.org (a@b)", "a@b", "faure@kde.org", true );
  // Interestingly, this isn't supported.
  //checkGetNameAndEmail( "\"a@b\" <faure@kde.org>", "a@b", "faure@kde.org", true );

  // While typing, when there's no '@' yet
  checkGetNameAndEmail( "foo", "foo", QString::null, false );
  checkGetNameAndEmail( "foo <", "foo", QString::null, false );
  checkGetNameAndEmail( "foo <b", "foo", "b", true );

  // If multiple emails are there, only return the first one
  checkGetNameAndEmail( "\"Faure, David\" <faure@kde.org>, KHZ <khz@khz.khz>", "Faure, David", "faure@kde.org", true );

  printf("\nTest OK !\n");

  return 0;
}

