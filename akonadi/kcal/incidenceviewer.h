/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company

    Author: Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_INCIDENCEVIEWER_H
#define AKONADI_INCIDENCEVIEWER_H

#include "akonadi-kcal_next_export.h"

#include <akonadi/itemmonitor.h>

#include <QtCore/QDate>
#include <QtGui/QWidget>

class QAbstractItemModel;

namespace Akonadi {

/**
 * @short A viewer component for incidences in Akonadi.
 *
 * This widgets provides a way to show an incidence from the
 * Akonadi storage.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * const Item item = ...
 *
 * IncidenceViewer *viewer = new IncidenceViewer( this );
 * viewer->setIncidence( item );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class AKONADI_KCAL_NEXT_EXPORT IncidenceViewer : public QWidget, public Akonadi::ItemMonitor
{
  Q_OBJECT

  public:
    /**
     * Creates a new incidence viewer.
     *
     * @param parent The parent widget.
     */
    IncidenceViewer( QWidget *parent = 0 );

    /**
     * Destroys the incidence viewer.
     */
    ~IncidenceViewer();

    /**
     * Returns the incidence that is currently displayed.
     */
    Akonadi::Item incidence() const;

    /**
     * Returns the active date used for the currently displayed incidence
     */
    QDate activeDate() const;
    
    /**
     * Returns the attachment model for the currently displayed incidence.
     */
    QAbstractItemModel *attachmentModel() const;

    /**
     * Sets whether the view shall be cleared as soon as an empty incidence is
     * set (default) or @p delayed when the next valid incidence is set.
     */
    void setDelayedClear( bool delayed );

    /**
     * Sets the default @p message that shall be shown if no incidence is set.
     */
    void setDefaultMessage( const QString &message );

    /**
     * Sets an additional @p text that is shown above the incidence.
     */
    void setHeaderText( const QString &text );

  public Q_SLOTS:
    /**
     * Sets the @p incidence that shall be displayed in the viewer.
     *
     * @param activeDate The active date is used to calculate the actual date of
     *                   the selected incidence in case of recurring incidences.
     */
    void setIncidence( const Akonadi::Item &incidence, const QDate &activeDate = QDate() );

  private:
    /**
     * This method is called whenever the displayed contact @p group has been changed.
     */
    virtual void itemChanged( const Item &group );

    /**
     * This method is called whenever the displayed contact group has been
     * removed from Akonadi.
     */
    virtual void itemRemoved();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotParentCollectionFetched( KJob* ) )
    //@endcond
};

}

#endif
