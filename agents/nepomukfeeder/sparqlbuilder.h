/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef SPARQLBUILDER_H
#define SPARQLBUILDER_H

#include <qstring.h>
#include <qurl.h>
#include <qvariant.h>

#include <boost/shared_ptr.hpp>


/**
  SPARQL query builder base class.
*/
class SparqlBuilder
{
  public:

    class BasicGraphPattern;

    /** A query variable, needed to avoid ambiguous calls to TriplePattern::setObject */
    class QueryVariable
    {
      public:
        explicit QueryVariable( const QString &variable ) : m_name( variable ) {}
        explicit QueryVariable( const char* variable ) : m_name( QString::fromAscii( variable ) ) {}

        QString name() const { return m_name; }
      private:
        QString m_name;
    };

    /** A SPARQL RDF triple pattern to be used in graph patterns. */
    class TriplePattern
    {
      public:
        template <typename Subject, typename Predicate, typename Object>
        TriplePattern( const Subject &subject, const Predicate &predicate, const Object &object )
        {
          setSubject( subject );
          setPredicate( predicate );
          setObject( object );
        }

        void setSubject( const QString &subject ) { m_subject = subject; }
        void setSubject( const QUrl &subject );
        void setPredicate( const QUrl &predicate );
        void setObject( const QVariant &object );
        void setObject( const QUrl &object );
        // ### this is still dangerous, way too easy to forget...
        void setObject( const QueryVariable &variable );
        // for resolving ambiguous calls between QVariant vs. QUrl due to implicit conversion from QString
        void setObject( const QString &object );
#ifndef QT_NO_CAST_FROM_ASCII
        void setObject( const char * object ) { setObject( QString( object ) ); }
#endif

      protected:
        QString toString() const;
        friend class SparqlBuilder::BasicGraphPattern;

      private:
        QString m_subject;
        QString m_predicate;
        QString m_object;
    };

    /** A SPARQL value constraint. */
    class ValueConstraint
    {
      public:
        explicit ValueConstraint( const QString &filter ) : m_filter( filter ) {}

      private:
        QString m_filter;
    };

    /** Base-class for SPARQL graph patterns. */
    class GraphPattern
    {
      public:
        GraphPattern() : m_isOptional( false ) {}
        virtual ~GraphPattern() {}
        void setName( const QString &name ) { m_name = name; }
        void setOptional( bool optional ) { m_isOptional = optional; }

      protected:
        QString toString() const;
        virtual QString toStringInternal() const = 0;
        friend class SparqlBuilder;

      private:
        QString m_name;
        bool m_isOptional;
    };

    /** A SPARQL basic graph pattern. */
    class BasicGraphPattern : public GraphPattern
    {
      public:
        BasicGraphPattern() {}
        BasicGraphPattern( const TriplePattern &triple ) { m_triples.append( triple ); }
        BasicGraphPattern( const QList<TriplePattern> &triples ) : m_triples( triples ) {}

        void addTriple( const TriplePattern &triple ) { m_triples.append( triple ); }
        template <typename Subject, typename Predicate, typename Object>
        void addTriple( const Subject &subject, const Predicate &predicate, const Object &object )
        {
          m_triples.append( TriplePattern( subject, predicate, object ) );
        }
        void addValueConstraint( const ValueConstraint &constraint ) { m_constraints.append( constraint ); }

        bool isEmpty() const { return m_triples.isEmpty() && m_constraints.isEmpty(); }

      protected:
        QString toStringInternal() const;

      private:
        QList<TriplePattern> m_triples;
        QList<ValueConstraint> m_constraints;
    };

    /** A SPARQL group graph pattern. */
    class GroupGraphPattern : public GraphPattern
    {
      public:
        GroupGraphPattern() : m_isUnion( false ) {}
        ~GroupGraphPattern() {}
        void setUnion( bool isUnion ) { m_isUnion = isUnion; }

        template <typename GraphPatternType>
        void addGraphPattern( const GraphPatternType &graph )
        {
          if ( !graph.isEmpty() )
            m_graphPatterns.append( boost::shared_ptr<GraphPattern>( new GraphPatternType( graph ) ) );
        }

        bool isEmpty() const { return m_graphPatterns.isEmpty(); }

      protected:
        QString toStringInternal() const;

      private:
        QList<boost::shared_ptr<GraphPattern> > m_graphPatterns;
        bool m_isUnion;
    };


    SparqlBuilder();
    virtual ~SparqlBuilder();

    /** Returns the SPARQL query as a string. */
    virtual QString query() const = 0;

    /** Sets the graph pattern of the query. */
    template <typename GraphPatternType>
    void setGraphPattern( const GraphPatternType &graphPattern )
    {
      m_graphPattern = boost::shared_ptr<GraphPattern>( new GraphPatternType( graphPattern ) );
    }

  protected:
    QString graphPattern() const;

  private:
    boost::shared_ptr<GraphPattern> m_graphPattern;
};

#endif
