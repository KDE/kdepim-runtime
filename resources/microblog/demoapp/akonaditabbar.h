/*
    Copyright (C) 2009 Omat Holding B.V. <info@omat.nl>

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

#ifndef __AKONADITABBAR_H__
#define __AKONADITABBAR_H__

#include <KTabBar>
#include <akonadi/collection.h>

/**
    This class is a KTabBar which you can use to show your top level folders of
    a resource. Call setResource() with your resource and it will create the tabs
    automatically. Whenever a click is made on a tab, it will emit a signal with
    the Collection.

    Example:
    @code
    
    m_tabBar = new AkonadiTabBar( box );
    connect( m_tabBar, SIGNAL( currentChanged( const Akonadi::Collection& ) ),
             SLOT( slotCurrentTabChanged( const Akonadi::Collection& ) ) );

    # and in some slot something like:
    const Akonadi::AgentInstanceModel *model = static_cast<const AgentInstanceModel*>( view->model() );
    const QString identifier = model->data( index, AgentInstanceModel::InstanceIdentifierRole ).toString();
    m_tabBar->setResource( identifier );

    @endcode
    @author Tom Albers
    @since 4.3
*/
class AkonadiTabBar : public KTabBar
{
    Q_OBJECT

public:
    /** Constructor */    
    AkonadiTabBar( QWidget* parent );
    /** Destructor */
    ~AkonadiTabBar();
    /**
     * Set the resource identifier which you want to use. The current tabs are removed and the new 
     * top-level folders of that resource are added again. If the 4th tab was selected previously, this
     * will be the one selected after the new tabs are created. In any case, the 
     * currentChanged( Akonadi::Collection ) will be emitted after this call. 
     */
    void setResource( const QString& );

signals:
    /** 
     * Signal emitted synchronisly with the currentChanged( int ) signal of KTabBar. This signal will 
     * also be emitted in all cases when setResource() has been called 
     */
    void currentChanged( Akonadi::Collection );

private:
    class Private;
    Private* d;
    Q_PRIVATE_SLOT( d, void slotCurrentChanged( int ) )
};

#endif
