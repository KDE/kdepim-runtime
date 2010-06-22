/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#ifndef KSELECTIONPROXYMODEL_H
#define KSELECTIONPROXYMODEL_H

#include <QtGui/QAbstractProxyModel>
#include <QtGui/QItemSelectionModel>


#include "kmodelindexproxymapper.h"
#include "kbihash_p.h"

typedef KBiHash<QPersistentModelIndex, QModelIndex> SourceProxyIndexMapping;
typedef KBiHash<qint64, QModelIndex> ParentMapping;

#define KDO(object) kDebug() << #object << object
#define SON(object) object->setObjectName(#object)

class QItemSelectionModel;

class KSelectionProxyModel;

class KSelectionProxyModelPrivate
{
public:
    KSelectionProxyModelPrivate(KSelectionProxyModel *model, QItemSelectionModel *selectionModel);

    Q_DECLARE_PUBLIC(KSelectionProxyModel)
    KSelectionProxyModel * const q_ptr;

    // A unique id is generated for each parent. It is used for the internalId of its children in the proxy
    // This is used to store a unique id for QModelIndexes in the proxy which have children.
    // If an index newly gets children it is added to this hash. If its last child is removed it is removed from this map.
    // If this map contains an index, that index hasChildren(). This hash is populated when new rows are inserted in the
    // source model, or a new selection is made.
    mutable ParentMapping m_parentIds;
    // This mapping maps indexes with children in the source to indexes with children in the proxy.
    // The order of indexes in this list is not relevant.
    mutable SourceProxyIndexMapping m_mappedParents;

    /**
      Keeping Persistent indexes from this model in this model breaks in certain situations
      such as after source insert, but before calling endInsertRows in this model. In such a state,
      the persistent indexes are not updated, but the methods assume they are already uptodate.

      Instead of using persistentindexes for proxy indexes in m_mappedParents, we maintain them ourselves with this method.

      m_mappedParents and m_parentIds are affected.

      @p parent and @p start refer to the proxy model. Any rows >= @p start will be updated.
      @p offset is the amount that affected indexes will be changed.
    */
    void updateInternalIndexes(const QModelIndex &parent, int start, int offset);

    /**
     * Updates stored indexes in the proxy. Any proxy row >= @p start is changed by @p offset.
     *
     * This is only called to update indexes in the top level of the proxy. Most commonly that is
     *
     * m_mappedParents, m_parentIds and m_mappedFirstChildren are affected.
     */
    void updateInternalTopIndexes(int start, int offset);

    void updateFirstChildMapping(const QModelIndex& parent, int offset);

    /**
      Creates mappings in m_parentIds and m_mappedParents between the source and the proxy.

      This is not recursive
    */
    void createParentMappings(const QModelIndex &parent, int start, int end) const;
    void createFirstChildMapping(const QModelIndex &parent, int proxyRow) const;
    bool firstChildAlreadyMapped(const QModelIndex &firstChild) const;
    bool parentAlreadyMapped(const QModelIndex &parent) const;
    void removeFirstChildMappings(int start, int end);
    void removeParentMappings(const QModelIndex &parent, int start, int end);

    /**
      Given a QModelIndex in the proxy, return the corresponding QModelIndex in the source.

      This method works only if the index has children in the proxy model which already has a mapping from the source.

      This means that if the proxy is a flat list, this method will always return QModelIndex(). Additionally, it means that m_mappedParents is not populated automatically and must be populated manually.

      No new mapping is created by this method.
    */
    QModelIndex mapParentToSource(const QModelIndex &proxyParent) const;

    /**
      Given a QModelIndex in the source model, return the corresponding QModelIndex in the proxy.

      This method works only if the index has children in the proxy model which already has a mapping from the source.

      No new mapping is created by this method.
    */
    QModelIndex mapParentFromSource(const QModelIndex &sourceParent) const;
    QModelIndex mapRootFirstChildToSource(const QModelIndex &proxyParent) const;
    QModelIndex mapRootFirstChildFromSource(const QModelIndex &sourceParent) const;

    // Only populated if m_startWithChildTrees.

    mutable SourceProxyIndexMapping m_mappedFirstChildren;

    // Source list is the selection in the source model.
    QList<QPersistentModelIndex> m_rootIndexList;

    KModelIndexProxyMapper *m_indexMapper;

    QPair<int, int> beginRemoveRows(const QModelIndex &parent, int start, int end) const;
    QPair<int, int> beginInsertRows(const QModelIndex &parent, int start, int end) const;
    void endRemoveRows(const QModelIndex &sourceParent, int proxyStart, int proxyEnd);
    void endInsertRows(const QModelIndex &parent, int start, int end);

    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destRow);
    void sourceRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destRow);
    void sourceModelAboutToBeReset();
    void sourceModelReset();
    void sourceLayoutAboutToBeChanged();
    void sourceLayoutChanged();
    void emitContinuousRanges(const QModelIndex &sourceFirst, const QModelIndex &sourceLast,
                              const QModelIndex &proxyFirst, const QModelIndex &proxyLast);
    void sourceDataChanged(const QModelIndex &topLeft , const QModelIndex &bottomRight);

    void refreshParentMappings(QList<QPersistentModelIndex> &mappings);
    void refreshFirstChildMappings(QList<QPersistentModelIndex> &mappings);

    void removeSelectionFromProxy(const QItemSelection &selection);
    void removeRangeFromProxy(const QItemSelectionRange &range);

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void sourceModelDestroyed();

    void resetInternalData();

    /**
      When items are inserted or removed in the m_startWithChildTrees configuration,
      this method helps find the startRow for use emitting the signals from the proxy.
    */
    int getProxyInitialRow(const QModelIndex &parent) const;

    /**
      If m_startWithChildTrees is true, this method returns the row in the proxy model to insert newIndex
      items.

      This is a special case because the items above rootListRow in the list are not in the model, but
      their children are. Those children must be counted.

      If m_startWithChildTrees is false, this method returns @p rootListRow.
    */
    int getTargetRow(int rootListRow);

    /**
      Inserts the indexes in @p list into the proxy model.
    */
    void insertSelectionIntoProxy(const QItemSelection& selection);

    /**
      Returns true if @p sourceIndex or one of its ascendants is already part of the proxy model.
    */
    bool isInModel(const QModelIndex &sourceIndex) const;

    bool m_startWithChildTrees;
    bool m_omitChildren;
    bool m_omitDescendants;
    bool m_includeAllSelected;
    bool m_rowsInserted;
    bool m_rowsRemoved;
    QPair<int, int> m_proxyRemoveRows;
    bool m_rowsMoved;
    bool m_resetting;
    bool m_ignoreNextLayoutAboutToBeChanged;
    bool m_ignoreNextLayoutChanged;
    QItemSelectionModel * const m_selectionModel;
    mutable qint64 m_nextId;

    int m_filterBehavior;

    QList<QPersistentModelIndex> m_layoutChangePersistentIndexes;
    QModelIndexList m_proxyIndexes;

    QModelIndexList m_weakSourceParents;
    QModelIndexList m_weakSourceFirstChilds;
    QModelIndexList m_weakRootIndexes;
    QModelIndexList m_weakRootParents;
    QList<QPersistentModelIndex> m_sourceFirstChildParents;
    QList<QPersistentModelIndex> m_sourceGrandParents;
    QList<QPersistentModelIndex> m_sourcePersistentParents;
    QList<QPersistentModelIndex> m_sourcePersistentFirstChilds;
};


/**
  @brief A Proxy Model which presents a subset of its source model to observers.

  The KSelectionProxyModel is most useful as a convenience for displaying the selection in one view in
  another view. The selectionModel of the initial view is used to create a proxied model which is filtered
  based on the configuration of this class.

  For example, when a user clicks a mail folder in one view in an email application, the contained emails
  should be displayed in another view.

  This takes away the need for the developer to handle the selection between the views, including all the
  mapToSource, mapFromSource and setRootIndex calls.

  @code
  MyModel *sourceModel = new MyModel(this);
  QTreeView *leftView = new QTreeView(this);
  leftView->setModel(sourceModel);

  KSelectionProxyModel *selectionProxy = new KSelectionProxyModel(leftView->selectionModel(), this);
  selectionProxy->setSourceModel(sourceModel);

  QTreeView *rightView = new QTreeView(this);
  rightView->setModel(selectionProxy);
  @endcode

  \image html selectionproxymodelsimpleselection.png "A Selection in one view creating a model for use with another view."

  The KSelectionProxyModel can handle complex selections.

  \image html selectionproxymodelmultipleselection.png "Non-contiguous selection creating a new simple model in a second view."

  The contents of the secondary view depends on the selection in the primary view, and the configuration of the proxy model.
  See KSelectionProxyModel::setFilterBehavior for the different possible configurations.

  For example, if the filterBehavior is SubTrees, selecting another item in an already selected subtree has no effect.

  \image html selectionproxymodelmultipleselection-withdescendant.png "Selecting an item and its descendant."

  See the test application in KDE/kdelibs/kdeui/tests/proxymodeltestapp to try out the valid configurations.

  \image html kselectionproxymodel-testapp.png "KSelectionProxyModel test application"

  Obviously, the KSelectionProxyModel may be used in a view, or further processed with other proxy models.
  See KAddressBook and AkonadiConsole in kdepim for examples which use a further KDescendantsProxyModel
  and QSortFilterProxyModel on top of a KSelectionProxyModel.

  Additionally, this class can be used to programmatically choose some items from the source model to display in the view. For example,
  this is how the Favourite Folder View in KMail works, and is also used in unit testing.

  See also: http://doc.trolltech.com/4.5/model-view-proxy-models.html

  @since 4.4
  @author Stephen Kelly <steveire@gmail.com>

*/
class KSelectionProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    /**
    ctor.

    @p selectionModel The selection model used to filter what is presented by the proxy.
    */

    explicit KSelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent = 0);

    /**
    dtor
    */
    virtual ~KSelectionProxyModel();

    /**
    reimp.
    */
    virtual void setSourceModel(QAbstractItemModel * sourceModel);

    QItemSelectionModel *selectionModel() const;

    enum FilterBehavior {
        SubTrees,
        SubTreeRoots,
        SubTreesWithoutRoots,
        ExactSelection,
        ChildrenOfExactSelection
    };
    Q_ENUMS(FilterBehavior)

    /**
      Set the filter behaviors of this model.
      The filter behaviors of the model govern the content of the model based on the selection of the contained QItemSelectionModel.

      See kdeui/proxymodeltestapp to try out the different proxy model behaviors.

      The most useful behaviors are SubTrees, ExactSelection and ChildrenOfExactSelection.

      The default behavior is SubTrees. This means that this proxy model will contain the roots of the items in the source model.
      Any descendants which are also selected have no additional effect.
      For example if the source model is like:

      @verbatim
      (root)
        - A
        - B
          - C
          - D
            - E
              - F
            - G
        - H
        - I
          - J
          - K
          - L
      @endverbatim

      And A, B, C and D are selected, the proxy will contain:

      @verbatim
      (root)
        - A
        - B
          - C
          - D
            - E
              - F
            - G
      @endverbatim

      That is, selecting 'D' or 'C' if 'B' is also selected has no effect. If 'B' is de-selected, then 'C' amd 'D' become top-level items:

      @verbatim
      (root)
        - A
        - C
        - D
          - E
            - F
          - G
      @endverbatim

      This is the behavior used by KJots when rendering books.

      If the behavior is set to SubTreeRoots, then the children of selected indexes are not part of the model. If 'A', 'B' and 'D' are selected,

      @verbatim
      (root)
        - A
        - B
      @endverbatim

      Note that although 'D' is selected, it is not part of the proxy model, because its parent 'B' is already selected.

      SubTreesWithoutRoots has the effect of not making the selected items part of the model, but making their children part of the model instead. If 'A', 'B' and 'I' are selected:

      @verbatim
      (root)
        - C
        - D
          - E
            - F
          - G
        - J
        - K
        - L
      @endverbatim

      Note that 'A' has no children, so selecting it has no outward effect on the model.

      ChildrenOfExactSelection causes the proxy model to contain the children of the selected indexes,but further descendants are omitted.
      Additionally, if descendants of an already selected index are selected, their children are part of the proxy model.
      For example, if 'A', 'B', 'D' and 'I' are selected:

      @verbatim
      (root)
        - C
        - D
        - E
        - G
        - J
        - K
        - L
      @endverbatim

      This would be useful for example if showing containers (for example maildirs) in one view and their items in another. Sub-maildirs would still appear in the proxy, but
      could be filtered out using a QSortfilterProxyModel.

      The ExactSelection behavior causes the selected items to be part of the proxy model, even if their ancestors are already selected, but children of selected items are not included.

      Again, if 'A', 'B', 'D' and 'I' are selected:

      @verbatim
      (root)
        - A
        - B
        - D
        - I
      @endverbatim

      This is the behavior used by the Favourite Folder View in KMail.

    */
    void setFilterBehavior(FilterBehavior behavior);
    FilterBehavior filterBehavior() const;

    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex & proxyIndex) const;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    virtual QMimeData* mimeData(const QModelIndexList & indexes) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

    virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex index(int, int, const QModelIndex& = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex&) const;
    virtual int columnCount(const QModelIndex& = QModelIndex()) const;

    virtual QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits = 1,
                                  Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const;

Q_SIGNALS:
#if !defined(Q_MOC_RUN)
private: // Don't allow subclasses to emit these signals.
#endif

    /**
      @internal
      Emitted before @p removeRootIndex, an index in the sourceModel is removed from
      the root selected indexes. This may be unrelated to rows removed from the model,
      depending on configuration.
    */
    void rootIndexAboutToBeRemoved(const QModelIndex &removeRootIndex);

    /**
      @internal
      Emitted when @p newIndex, an index in the sourceModel is added to the root selected
      indexes. This may be unrelated to rows inserted to the model,
      depending on configuration.
    */
    void rootIndexAdded(const QModelIndex &newIndex);

    /**
      @internal
      Emitted before @p selection, a selection in the sourceModel, is removed from
      the root selection.
    */
    void rootSelectionAboutToBeRemoved(const QItemSelection &selection);

    /**
      @internal
      Emitted after @p selection, a selection in the sourceModel, is added to
      the root selection.
    */
    void rootSelectionAdded(const QItemSelection &selection);

protected:
    QList<QPersistentModelIndex> sourceRootIndexes() const;

private:
    Q_DECLARE_PRIVATE(KSelectionProxyModel)
    //@cond PRIVATE
    KSelectionProxyModelPrivate *d_ptr;

    Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeInserted(const QModelIndex &, int, int))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsInserted(const QModelIndex &, int, int))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeRemoved(const QModelIndex &, int, int))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsRemoved(const QModelIndex &, int, int))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int))
    Q_PRIVATE_SLOT(d_func(), void sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int))
    Q_PRIVATE_SLOT(d_func(), void sourceModelAboutToBeReset())
    Q_PRIVATE_SLOT(d_func(), void sourceModelReset())
    Q_PRIVATE_SLOT(d_func(), void sourceLayoutAboutToBeChanged())
    Q_PRIVATE_SLOT(d_func(), void sourceLayoutChanged())
    Q_PRIVATE_SLOT(d_func(), void sourceDataChanged(const QModelIndex &, const QModelIndex &))
    Q_PRIVATE_SLOT(d_func(), void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected))
    Q_PRIVATE_SLOT(d_func(), void sourceModelDestroyed())


    //@endcond

};

#endif
