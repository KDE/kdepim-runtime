/*
  This file is part of libkdepim.

  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#ifndef KCONFIGPROPAGATOR_H
#define KCONFIGPROPAGATOR_H

#include <QString>

#include <qdom.h>
#include <q3ptrlist.h>
#include <QList>
#include <kdepim_export.h>

class KConfigSkeleton;
class KConfigSkeletonItem;

namespace KPIM {

class KDEPIM_EXPORT KConfigPropagator
{
  public:

    /**
      Create KConfigPropagator object without associated source configuration.
    */
    KConfigPropagator();
    /**
      Create KConfigPropagator object.

      @param skeleton KConfigSkeleton object used as source for the propagation
      @param kcfgFile file name of kcfg file containing the propagation rules
    */
    KConfigPropagator( KConfigSkeleton *skeleton, const QString &kcfgFile );
    virtual ~KConfigPropagator() {}

    KConfigSkeleton *skeleton() { return mSkeleton; }

    /*
      Commit changes according to propagation rules.
    */
    void commit();

    class KDEPIM_EXPORT Condition
    {
      public:
        Condition() : isValid( false ) {}

        QString file;
        QString group;
        QString key;
        QString value;

        bool isValid;
    };

    class KDEPIM_EXPORT Rule
    {
      public:
        typedef QList<Rule> List;

        Rule() : hideValue( false ) {}

        QString sourceFile;
        QString sourceGroup;
        QString sourceEntry;

        QString targetFile;
        QString targetGroup;
        QString targetEntry;

        Condition condition;

        bool hideValue;
    };

    class KDEPIM_EXPORT Change
    {
      public:
        typedef Q3PtrList<Change> List;

        Change( const QString &title ) : mTitle( title ) {}
        virtual ~Change();

        void setTitle( const QString &title ) { mTitle = title; }
        QString title() const { return mTitle; }

        virtual QString arg1() const { return QString(); }
        virtual QString arg2() const { return QString(); }

        virtual void apply() = 0;

      private:
        QString mTitle;
    };

    class KDEPIM_EXPORT ChangeConfig : public Change
    {
      public:
        ChangeConfig();
        ~ChangeConfig() {}

        QString arg1() const;
        QString arg2() const;

        void apply();

        QString file;
        QString group;
        QString name;
        QString label;
        QString value;
        bool hideValue;
    };

    void updateChanges();

    Change::List changes();

    Rule::List rules();

  protected:
    void init();

    /**
      Implement this function in a subclass if you want to add changes which
      can't be expressed as propagations in the kcfg file.
    */
    virtual void addCustomChanges( Change::List & ) {}

    KConfigSkeletonItem *findItem( const QString &group, const QString &name );

    QString itemValueAsString( KConfigSkeletonItem * );

    void readKcfgFile();

    Rule parsePropagation( const QDomElement &e );
    Condition parseCondition( const QDomElement &e );

    void parseConfigEntryPath( const QString &path, QString &file,
                               QString &group, QString &entry );

  private:
    KConfigSkeleton *mSkeleton;
    QString mKcfgFile;

    Rule::List mRules;
    Change::List mChanges;
};

}

#endif
