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

#include "kselectionproxymodel.h"

#include <QtCore/QStack>
#include <QtCore/QStringList>
#include <QtGui/QItemSelectionRange>

/**
  Return true if @p idx is a descendant of one of the indexes in @p list.
  Note that this returns false if @p list contains @p idx.
*/
template<typename ModelIndex>
bool isDescendantOf(const QList<ModelIndex> &list, const QModelIndex &idx)
{
    if (!idx.isValid())
        return false;

    if (list.contains(idx))
        return false;

    QModelIndex parent = idx.parent();
    while (parent.isValid()) {
        if (list.contains(parent))
            return true;
        parent = parent.parent();
    }
    return false;
}

static bool isDescendantOf(const QModelIndex &ancestor, const QModelIndex &descendant)
{
    if (!descendant.isValid())
        return false;

    if (ancestor == descendant)
        return false;

    QModelIndex parent = descendant.parent();
    while (parent.isValid()) {
        if (parent == ancestor)
            return true;

        parent = parent.parent();
    }
    return false;
}

static bool isDescendantOf(const QItemSelection &selection, const QModelIndex &descendant)
{
    if (!descendant.isValid())
        return false;

    if (selection.contains(descendant))
        return false;

    QModelIndex parent = descendant.parent();
    while (parent.isValid()) {
        if (selection.contains(parent))
            return true;

        parent = parent.parent();
    }
    return false;
}

static bool isDescendantOf(const QItemSelectionRange &range, const QModelIndex &descendant)
{
    if (!descendant.isValid())
        return false;

    if (range.contains(descendant))
        return false;

    QModelIndex parent = descendant.parent();
    while (parent.isValid()) {
        if (range.contains(parent))
            return true;

        parent = parent.parent();
    }
    return false;
}

static int _getRootListRow(const QList<QModelIndexList> &rootAncestors, const QModelIndex &index)
{
    QModelIndex commonParent = index;
    QModelIndex youngestAncestor;

    int firstCommonParent = -1;
    int bestParentRow = -1;
    while (commonParent.isValid()) {
        youngestAncestor = commonParent;
        commonParent = commonParent.parent();

        for (int i = 0; i < rootAncestors.size(); ++i) {
            const QModelIndexList ancestorList = rootAncestors.at(i);

            const int parentRow = ancestorList.indexOf(commonParent);

            if (parentRow < 0)
                continue;

            if (parentRow > bestParentRow) {
                firstCommonParent = i;
                bestParentRow = parentRow;
            }
        }

        if (firstCommonParent >= 0)
            break;
    }

    // If @p list is non-empty, the invalid QModelIndex() will at least be found in ancestorList.
    Q_ASSERT(firstCommonParent >= 0);

    const QModelIndexList firstAnsList = rootAncestors.at(firstCommonParent);

    const QModelIndex eldestSibling = firstAnsList.value(bestParentRow + 1);

    if (eldestSibling.isValid()) {
        // firstCommonParent is a sibling of one of the ancestors of @p index.
        // It is the first index to share a common parent with one of the ancestors of @p index.
        if (eldestSibling.row() >= youngestAncestor.row())
            return firstCommonParent;
    }

    int siblingOffset = 1;

    // The same commonParent might be common to several root indexes.
    // If this is the last in the list, it's the only match. We instruct the model
    // to insert the new index after it ( + siblingOffset).
    if (rootAncestors.size() <= firstCommonParent + siblingOffset) {
        return firstCommonParent + siblingOffset;
    }

    // A
    // - B
    //   - C
    //   - D
    //   - E
    // F
    //
    // F is selected, then C then D. When inserting D into the model, the commonParent is B (the parent of C).
    // The next existing sibling of B is F (in the proxy model). bestParentRow will then refer to an index on
    // the level of a child of F (which doesn't exist - Boom!). If it doesn't exist, then we've already found
    // the place to insert D
    QModelIndexList ansList = rootAncestors.at(firstCommonParent + siblingOffset);
    if (ansList.size() <= bestParentRow) {
        return firstCommonParent + siblingOffset;
    }

    QModelIndex nextParent = ansList.at(bestParentRow);
    while (nextParent == commonParent) {
        if (ansList.size() < bestParentRow + 1)
            // If the list is longer, it means that at the end of it is a descendant of the new index.
            // We insert the ancestors items first in that case.
            break;

        const QModelIndex nextSibling = ansList.value(bestParentRow + 1);

        if (!nextSibling.isValid()) {
            continue;
        }

        if (youngestAncestor.row() <= nextSibling.row()) {
            break;
        }

        siblingOffset++;

        if (rootAncestors.size() <= firstCommonParent + siblingOffset)
            break;

        ansList = rootAncestors.at(firstCommonParent + siblingOffset);

        // In the scenario above, E is selected after D, causing this loop to be entered,
        // and requiring a similar result if the next sibling in the proxy model does not have children.
        if (ansList.size() <= bestParentRow) {
            break;
        }

        nextParent = ansList.at(bestParentRow);
    }

    return firstCommonParent + siblingOffset;
}

/**
  Determines the correct location to insert @p index into @p list.
*/
template<typename ModelIndex>
static int getRootListRow(const QList<ModelIndex> &list, const QModelIndex &index)
{
    if (!index.isValid())
        return -1;

    if (list.isEmpty())
        return 0;

    // What's going on?
    // Consider a tree like
    //
    // A
    // - B
    // - - C
    // - - - D
    // - E
    // - F
    // - - G
    // - - - H
    // - I
    // - - J
    // - K
    //
    // If D, E and J are already selected, and H is newly selected, we need to put H between E and J in the proxy model.
    // To figure that out, we create a list for each already selected index of its ancestors. Then,
    // we climb the ancestors of H until we reach an index with siblings which have a descendant
    // selected (F above has siblings B, E and I which have descendants which are already selected).
    // Those child indexes are traversed to find the right sibling to put F beside.
    //
    // i.e., new items are inserted in the expected location.

    QList<QModelIndexList> rootAncestors;
    foreach(const QModelIndex &root, list) {
        QModelIndexList ancestors;
        ancestors << root;
        QModelIndex parent = root.parent();
        while (parent.isValid()) {
            ancestors.prepend(parent);
            parent = parent.parent();
        }
        ancestors.prepend(QModelIndex());
        rootAncestors << ancestors;
    }
    return _getRootListRow(rootAncestors, index);
}

/**
  Returns a selection in which no descendants of selected indexes are also themselves selected.
  For example,
  @code
    A
    - B
    C
    D
  @endcode
  If A, B and D are selected in @p selection, the returned selection contains only A and D.
*/
static QItemSelection getRootRanges(const QItemSelection &_selection)
{
    QItemSelection rootSelection;
    QItemSelection selection = _selection;
    QList<QItemSelectionRange>::iterator it = selection.begin();
    while (it != selection.end()) {
        if (!it->topLeft().parent().isValid())
        {
            rootSelection.append(*it);
            it = selection.erase(it);
        } else
            ++it;
    }

    it = selection.begin();
    const QList<QItemSelectionRange>::iterator end = selection.end();
    while ( it != end ) {
        const QItemSelectionRange range = *it;
        it = selection.erase(it);

        if (isDescendantOf(rootSelection, range.topLeft()) || isDescendantOf(selection, range.topLeft()))
            continue;

        rootSelection << range;
    }
    return rootSelection;
}

template <typename ForwardIterator>
ForwardIterator kMaxElement(ForwardIterator it, ForwardIterator end)
{
    ForwardIterator result = it;
    for ( ; it != end; ++it)
    {
        if (*result < *it)
            result = it;
    }
    return result;
}
/**
 */
struct RangeLessThan
{
    bool operator()(const QItemSelectionRange &left, const QItemSelectionRange &right)
    {
        if (right.model() == left.model()) {
            // parent has to be calculated, so we only do so once.
            const QModelIndex topLeftParent = left.parent();
            const QModelIndex otherTopLeftParent = right.parent();
            if (topLeftParent == otherTopLeftParent) {
                if (right.top() == left.top()) {
                    if (right.left() == left.left()) {
                        if (right.bottom() == left.bottom()) {
                            return left.right() < right.right();
                        }
                        return left.bottom() < right.bottom();
                    }
                    return left.left() < right.left();
                }
                return left.top() < right.top();
            }
            return topLeftParent < otherTopLeftParent;
        }
        return left.model() < right.model();
    }
};

static QItemSelection stableNormalizeSelection(QItemSelection selection)
{
    if (selection.size() <= 1)
        return selection;

    QList<QItemSelectionRange>::iterator it = selection.begin();

    Q_ASSERT(it != selection.end());
    QList<QItemSelectionRange>::iterator scout = it + 1;
    while (scout != selection.end()) {
        Q_ASSERT(it != selection.end());
        int bottom = it->bottom();
        while (scout != selection.end() && it->parent() == scout->parent() && bottom + 1 == scout->top()) {
            bottom = scout->bottom();
            scout = selection.erase(scout);
        }
        if (bottom != it->bottom()) {
            const QModelIndex topLeft = it->topLeft();
            *it = QItemSelectionRange(topLeft, topLeft.sibling(bottom, it->right()));
        }
        Q_ASSERT(it != scout);
        if (scout == selection.end())
            break;
        it = scout;
        ++scout;
    }
    return selection;
}

static QItemSelection normalizeSelection(QItemSelection selection)
{
    if (selection.size() <= 1)
        return selection;

    RangeLessThan lt;
    qSort(selection.begin(), selection.end(), lt);
    return stableNormalizeSelection(selection);
}


KSelectionProxyModelPrivate::KSelectionProxyModelPrivate(KSelectionProxyModel* model, QItemSelectionModel* selectionModel)
      : q_ptr(model),
        m_startWithChildTrees(false),
        m_omitChildren(false),
        m_omitDescendants(false),
        m_includeAllSelected(false),
        m_rowsInserted(false),
        m_rowsRemoved(false),
        m_rowsMoved(false),
        m_resetting(false),
        m_ignoreNextLayoutAboutToBeChanged(false),
        m_ignoreNextLayoutChanged(false),
        m_selectionModel(selectionModel),
        m_nextId(1)
{
  // QItemSelectionModel doesn't clear its selection when its model is reset so we do it manually here.
  // Fixed in Qt 4.7: http://qt.gitorious.org/qt/qt/merge_requests/639
  QObject::connect(selectionModel->model(), SIGNAL(modelAboutToBeReset()), selectionModel, SLOT(clear()));
}

void KSelectionProxyModelPrivate::emitContinuousRanges(const QModelIndex &sourceFirst, const QModelIndex &sourceLast,
                                                       const QModelIndex &proxyFirst, const QModelIndex &proxyLast)
{
    Q_Q(KSelectionProxyModel);

    const int proxyRangeSize = proxyLast.row() - proxyFirst.row();
    const int sourceRangeSize = sourceLast.row() - sourceFirst.row();

    if (proxyRangeSize == sourceRangeSize) {
        emit q->dataChanged(proxyFirst, proxyLast);
        return;
    }


    // TODO: Loop to skip descendant ranges.
//     int lastRow;
//
//     const QModelIndex sourceHalfWay = sourceFirst.sibling(sourceFirst.row() + (sourceRangeSize / 2));
//     const QModelIndex proxyHalfWay = proxyFirst.sibling(proxyFirst.row() + (proxyRangeSize / 2));
//     const QModelIndex mappedSourceHalfway = q->mapToSource(proxyHalfWay);
//
//     const int halfProxyRange = mappedSourceHalfway.row() - proxyFirst.row();
//     const int halfSourceRange = sourceHalfWay.row() - sourceFirst.row();
//
//     if (proxyRangeSize == sourceRangeSize)
//     {
//         emit q->dataChanged(proxyFirst, proxyLast.sibling(proxyFirst.row() + proxyRangeSize, proxyLast.column()));
//         return;
//     }

    emit q->dataChanged(proxyFirst, proxyLast);
}

void KSelectionProxyModelPrivate::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_Q(KSelectionProxyModel);

    const QModelIndex proxyTopLeft = q->mapFromSource(topLeft);
    const QModelIndex proxyBottomRight = q->mapFromSource(bottomRight);

    const QModelIndex proxyRangeParent = proxyTopLeft.parent();

    if (!m_omitChildren && m_omitDescendants && m_startWithChildTrees && m_includeAllSelected) {
        // ChildrenOfExactSelection
        if (proxyTopLeft.isValid())
            emitContinuousRanges(topLeft, bottomRight, proxyTopLeft, proxyBottomRight);
        return;
    }

    if ((m_omitChildren && !m_startWithChildTrees && m_includeAllSelected)
            || (!proxyRangeParent.isValid() && !m_startWithChildTrees)) {
        // Exact selection and SubTreeRoots and SubTrees in top level
        // Emit continuous ranges.
        QList<int> changedRows;
        for (int row = topLeft.row(); row < bottomRight.row(); ++row) {
            const QModelIndex index = q->sourceModel()->index(row, topLeft.column(), topLeft.parent());
            const int idx = m_rootIndexList.indexOf(index);
            if (idx != -1) {
                changedRows.append(idx);
            }
        }
        if (changedRows.isEmpty())
            return;
        int first = changedRows.first();
        int previous = first;
        QList<int>::const_iterator it = changedRows.constBegin();
        const QList<int>::const_iterator end = changedRows.constEnd();
        for ( ; it != end; ++it) {
            if (*it == previous + 1) {
                ++previous;
            } else {
                const QModelIndex _top = q->index(first, topLeft.column());
                const QModelIndex _bottom = q->index(previous, bottomRight.column());
                emit q->dataChanged(_top, _bottom);
                previous = first = *it;
            }
        }
        if (first != previous) {
            const QModelIndex _top = q->index(first, topLeft.column());
            const QModelIndex _bottom = q->index(previous, bottomRight.column());
            emit q->dataChanged(_top, _bottom);
        }
        return;
    }
    if (proxyRangeParent.isValid()) {
        if (m_omitChildren && !m_startWithChildTrees && !m_includeAllSelected)
            // SubTreeRoots
            return;
        if (!proxyTopLeft.isValid())
            return;
        // SubTrees and SubTreesWithoutRoots
        emit q->dataChanged(proxyTopLeft, proxyBottomRight);
        return;
    }

    if (m_startWithChildTrees && !m_omitChildren && !m_includeAllSelected && !m_omitDescendants) {
        // SubTreesWithoutRoots
        if (proxyTopLeft.isValid())
            emit q->dataChanged(proxyTopLeft, proxyBottomRight);
        return;
    }
}

void KSelectionProxyModelPrivate::sourceLayoutAboutToBeChanged()
{
    Q_Q(KSelectionProxyModel);

    if (m_ignoreNextLayoutAboutToBeChanged) {
        m_ignoreNextLayoutAboutToBeChanged = false;
        return;
    }

    if (!m_selectionModel->hasSelection())
        return;

    emit q->layoutAboutToBeChanged();

    QPersistentModelIndex srcPersistentIndex;
    foreach(const QPersistentModelIndex &proxyPersistentIndex, q->persistentIndexList()) {
        m_proxyIndexes << proxyPersistentIndex;
        Q_ASSERT(proxyPersistentIndex.isValid());
        srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
        Q_ASSERT(srcPersistentIndex.isValid());
        m_layoutChangePersistentIndexes << srcPersistentIndex;
    }

    SourceProxyIndexMapping::left_const_iterator parentIt = m_mappedParents.leftConstBegin();
    const SourceProxyIndexMapping::left_const_iterator parentEnd = m_mappedParents.leftConstEnd();
    for ( ; parentIt != parentEnd; ++parentIt) {
        m_sourcePersistentParents << parentIt.key();
        m_weakSourceParents << parentIt.key();
        m_sourceGrandParents << parentIt.key().parent();
    }
    SourceProxyIndexMapping::left_const_iterator firstChildIt = m_mappedFirstChildren.leftConstBegin();
    const SourceProxyIndexMapping::left_const_iterator firstChildEnd = m_mappedFirstChildren.leftConstEnd();
    for ( ; firstChildIt != firstChildEnd; ++firstChildIt) {
        m_sourcePersistentFirstChilds << firstChildIt.key();
        m_weakSourceFirstChilds << firstChildIt.key();
        m_sourceFirstChildParents << firstChildIt.key().parent();
    }
    QList<QPersistentModelIndex>::const_iterator rootIt = m_rootIndexList.constBegin();
    const QList<QPersistentModelIndex>::const_iterator rootEnd = m_rootIndexList.constEnd();
    for ( ; rootIt != rootEnd; ++rootIt) {
        m_weakRootIndexes << *rootIt;
        m_weakRootParents << rootIt->parent();
    }
}

void KSelectionProxyModelPrivate::refreshFirstChildMappings(QList<QPersistentModelIndex> &mappings)
{
    Q_Q(KSelectionProxyModel);

    QList<QPersistentModelIndex>::const_iterator it = mappings.constBegin();
    const QList<QPersistentModelIndex>::const_iterator end = mappings.constEnd();

    for ( ; it != end; ++it) {
        if (!it->isValid()) {
            m_mappedFirstChildren.removeLeft(*it);
            continue;
        }
        if (!it->parent().isValid())
            continue;

        Q_ASSERT(m_mappedFirstChildren.leftContains(*it));
        m_mappedFirstChildren.removeLeft(*it);

        int _newProxyRow = 0;
        const int rootListRow = m_rootIndexList.indexOf(it->parent());
        for (int i = 0; i < rootListRow; ++i)
            _newProxyRow += q->sourceModel()->rowCount(m_rootIndexList.at(i));

        createFirstChildMapping(it->parent(), _newProxyRow);
    }
}

void KSelectionProxyModelPrivate::refreshParentMappings(QList<QPersistentModelIndex> &mappings)
{
    Q_Q(KSelectionProxyModel);

    const int lastSize = mappings.size();

    QList<QPersistentModelIndex>::iterator it = mappings.begin();

    while (it != mappings.end()) {
        if (!it->isValid()) {
            m_parentIds.removeRight(m_mappedParents.takeLeft(*it));
            it = mappings.erase(it);
            continue;
        }
        if (it->parent().isValid() || m_mappedParents.leftContains(it->parent()) || m_rootIndexList.contains(*it)) {
            const qint64 id = m_parentIds.takeRight(m_mappedParents.takeLeft(*it));
            const QModelIndex newProxyIndex = q->mapFromSource(*it);
            m_mappedParents.insert(*it, newProxyIndex);
            m_parentIds.insert(id, newProxyIndex);

            it = mappings.erase(it);
        } else
            ++it;
    }
    if (!mappings.isEmpty()) {
        if (lastSize == mappings.size()) {
            Q_ASSERT(!"Something went very wrong");
            return;
        }
        refreshParentMappings(mappings);
    }
}

void KSelectionProxyModelPrivate::sourceLayoutChanged()
{
    Q_Q(KSelectionProxyModel);

    if (m_ignoreNextLayoutChanged) {
        m_ignoreNextLayoutChanged = false;
        return;
    }

    if (!m_selectionModel->hasSelection()) {
        return;
    }

    QList<QPersistentModelIndex> parentMappingsToUpdate;
    for (int i = 0; i < m_sourcePersistentParents.size(); ++i) {
        const QPersistentModelIndex persistentIndex = m_sourcePersistentParents.at(i);
        const QModelIndex nonPersistentIndex = m_weakSourceParents.at(i);
        if (persistentIndex.row() == nonPersistentIndex.row()
                && persistentIndex.column() == nonPersistentIndex.column()
                && persistentIndex.parent() == m_sourceGrandParents.at(i))
            continue;

        parentMappingsToUpdate << persistentIndex;
    }
    if (!parentMappingsToUpdate.isEmpty())
        refreshParentMappings(parentMappingsToUpdate);

    QList<QPersistentModelIndex> changedRoots;
    QList<QPersistentModelIndex>::iterator rootIt = m_rootIndexList.begin();
    const QList<QPersistentModelIndex>::iterator rootEnd = m_rootIndexList.end();
    int rootPosition = 0;
    while (rootIt != rootEnd) {
        const QPersistentModelIndex persistentIndex = *rootIt;
        const QModelIndex nonPersistentIndex = m_weakRootIndexes.at(rootPosition);
        const QModelIndex weakParent = m_weakRootParents.at(rootPosition);
        ++rootPosition;
        if (persistentIndex.row() == nonPersistentIndex.row()
                && persistentIndex.column() == nonPersistentIndex.column()
                && persistentIndex.parent() == weakParent) {
            ++rootIt;
            continue;
        }

        if (persistentIndex.isValid())
            changedRoots << persistentIndex;

        rootIt = m_rootIndexList.erase(rootIt);
    }
    foreach(const QPersistentModelIndex &idx, changedRoots) {
        int row = getRootListRow(m_rootIndexList, idx);
        m_rootIndexList.insert(row, idx);
    }

    QList<QPersistentModelIndex> firstChildsToUpdate;
    for (int i = 0; i < m_sourcePersistentFirstChilds.size(); ++i) {
        const QPersistentModelIndex persistentIndex = m_sourcePersistentFirstChilds.at(i);
        const QModelIndex nonPersistentIndex = m_weakSourceFirstChilds.at(i);
        if (persistentIndex.row() == nonPersistentIndex.row()
                && persistentIndex.column() == nonPersistentIndex.column()
                && persistentIndex.parent() == m_sourceFirstChildParents.at(i))
            continue;

        firstChildsToUpdate << persistentIndex;
    }
    if (!firstChildsToUpdate.isEmpty())
        refreshFirstChildMappings(firstChildsToUpdate);

    for (int i = 0; i < m_proxyIndexes.size(); ++i) {
        // Need to update m_mappedParents and m_mappedFirstChildren and m_parentIds before trying mapFromSource.
        q->changePersistentIndex(m_proxyIndexes.at(i), q->mapFromSource(m_layoutChangePersistentIndexes.at(i)));
    }

    m_layoutChangePersistentIndexes.clear();
    m_proxyIndexes.clear();

    m_weakSourceParents.clear();
    m_weakSourceFirstChilds.clear();
    m_weakRootIndexes.clear();
    m_weakRootParents.clear();
    m_sourceFirstChildParents.clear();
    m_sourceGrandParents.clear();
    m_sourcePersistentParents.clear();
    m_sourcePersistentFirstChilds.clear();

    emit q->layoutChanged();
}

void KSelectionProxyModelPrivate::resetInternalData()
{
    m_rootIndexList.clear();
    m_layoutChangePersistentIndexes.clear();
    m_proxyIndexes.clear();
    m_weakSourceParents.clear();
    m_weakSourceFirstChilds.clear();
    m_weakRootIndexes.clear();
    m_weakRootParents.clear();
    m_sourceFirstChildParents.clear();
    m_sourceGrandParents.clear();
    m_sourcePersistentParents.clear();
    m_sourcePersistentFirstChilds.clear();
    m_mappedParents.clear();
    m_parentIds.clear();
    m_mappedFirstChildren.clear();
    m_nextId = 1;
}

void KSelectionProxyModelPrivate::sourceModelDestroyed()
{
    Q_Q(KSelectionProxyModel);
    // There is very little we can do here.
    resetInternalData();
    m_resetting = false;
    q->endResetModel();
}

void KSelectionProxyModelPrivate::sourceModelAboutToBeReset()
{
    Q_Q(KSelectionProxyModel);

    // Deselecting an index in the selectionModel will cause it to
    // be removed from m_rootIndexList, so we don't need to clear
    // the list here manually.
    // We also don't need to notify that an index is about to be removed.
    m_selectionModel->clearSelection();

    q->beginResetModel();
    m_resetting = true;
}

void KSelectionProxyModelPrivate::sourceModelReset()
{
    Q_Q(KSelectionProxyModel);

    // No need to try to refill this. When the model is reset it doesn't have a meaningful selection anymore,
    // but when it gets one we'll be notified anyway.
    resetInternalData();
    m_resetting = false;
    q->endResetModel();
}

int KSelectionProxyModelPrivate::getProxyInitialRow(const QModelIndex &parent) const
{
    Q_ASSERT(m_rootIndexList.contains(parent));

    // The difficulty here is that parent and parent.parent() might both be in the m_rootIndexList.

    // - A
    // - B
    // - - C
    // - - D
    // - - - E

    // Consider that B and D are selected. The proxy model is:

    // - C
    // - D
    // - E

    // Then D gets a new child at 0. In that case we require adding F between D and E.

    // Consider instead that D gets removed. Then @p parent will be B.


    Q_Q(const KSelectionProxyModel);
    int parentPosition = m_rootIndexList.indexOf(parent);

    QModelIndex parentAbove;

    // If parentPosition == 0, then parent.parent() is not also in the model. (ordering is preserved)
    while (parentPosition > 0) {
        parentPosition--;

        parentAbove = m_rootIndexList.at(parentPosition);
        Q_ASSERT(parentAbove.isValid());

        int rows = q->sourceModel()->rowCount(parentAbove);
        if (rows > 0) {
            QModelIndex sourceIndexAbove = q->sourceModel()->index(rows - 1, 0, parentAbove);
            Q_ASSERT(sourceIndexAbove.isValid());
            QModelIndex proxyChildAbove = q->mapFromSource(sourceIndexAbove);
            Q_ASSERT(proxyChildAbove.isValid());
            return proxyChildAbove.row() + 1;
        }
    }
    return 0;
}

void KSelectionProxyModelPrivate::updateFirstChildMapping(const QModelIndex& parent, int offset)
{
    Q_Q(KSelectionProxyModel);

    static const int column = 0;
    static const int row = 0;

    const QPersistentModelIndex srcIndex = q->sourceModel()->index(row, column, parent);

    const QPersistentModelIndex previousFirstChild = q->sourceModel()->index(offset, column, parent);

    SourceProxyIndexMapping::left_iterator it = m_mappedFirstChildren.findLeft(previousFirstChild);
    if (it == m_mappedFirstChildren.leftEnd())
        return;

    Q_ASSERT(srcIndex.isValid());
    const QModelIndex proxyIndex = it.value();
    Q_ASSERT(proxyIndex.isValid());
    Q_ASSERT(proxyIndex.model() == q);

    m_mappedFirstChildren.eraseLeft(it);

    // The proxy index in the mapping has already been updated by the offset in updateInternalTopIndexes
    // so we restore it by applying the reverse.
    const QModelIndex newProxyIndex = q->createIndex(proxyIndex.row() - offset, proxyIndex.column());
    m_mappedFirstChildren.insert(srcIndex, newProxyIndex);
}

QPair< int, int > KSelectionProxyModelPrivate::beginInsertRows(const QModelIndex& parent, int start, int end) const
{
    Q_Q(const KSelectionProxyModel);

    const QModelIndex proxyParent = q->mapFromSource(parent);

    if (!m_startWithChildTrees) {
        // SubTrees
        if (proxyParent.isValid())
            return qMakePair(start, end);
        return qMakePair(-1, -1);
    }

    if (!m_includeAllSelected && proxyParent.isValid()) {
        // SubTreesWithoutRoots deeper than topLevel
        return qMakePair(start, end);
    }

    if (!m_rootIndexList.contains(parent))
        return qMakePair(-1, -1);

    const int proxyStartRow = getProxyInitialRow(parent) + start;
    return qMakePair(proxyStartRow, proxyStartRow + (end - start));
}

void KSelectionProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_Q(KSelectionProxyModel);

    if (!m_selectionModel->hasSelection())
        return;

    if (m_omitChildren)
        // ExactSelection and SubTreeRoots
        return;

    // topLevel insertions can be ignored because topLevel items would need to be selected to affect the proxy.
    if (!parent.isValid())
        return;

    QPair<int, int> pair = beginInsertRows(parent, start, end);
    if (pair.first == -1)
        return;

    const QModelIndex proxyParent = m_startWithChildTrees ? QModelIndex() : q->mapFromSource(parent);

    m_rowsInserted = true;
    q->beginInsertRows(proxyParent, pair.first, pair.second);
}

void KSelectionProxyModelPrivate::endInsertRows(const QModelIndex& parent, int start, int end)
{
    Q_Q(const KSelectionProxyModel);
    const QModelIndex proxyParent = q->mapFromSource(parent);
    const int offset = end - start + 1;

    const bool isNewParent = (q->sourceModel()->rowCount(parent) == offset);

    if (m_startWithChildTrees && m_rootIndexList.contains(parent)) {
        const int proxyInitialRow = getProxyInitialRow(parent);
        Q_ASSERT(proxyInitialRow >= 0);
        const int proxyStartRow = proxyInitialRow + start;

        updateInternalTopIndexes(proxyStartRow, offset);
        if (isNewParent)
            createFirstChildMapping(parent, proxyStartRow);
        else if (start == 0)
            // We already have a first child mapping, but what we have mapped is not the first child anymore
            // so we need to update it.
            updateFirstChildMapping(parent, end + 1);
    } else {
        Q_ASSERT(proxyParent.isValid());
        if (!isNewParent)
            updateInternalIndexes(proxyParent, start, offset);
        else
            createParentMappings(parent.parent(), parent.row(), parent.row());
    }
    createParentMappings(parent, start, end);
}

void KSelectionProxyModelPrivate::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_Q(KSelectionProxyModel);

    if (!m_rowsInserted)
        return;
    m_rowsInserted = false;
    endInsertRows(parent, start, end);
    q->endInsertRows();
}

QPair<int, int> KSelectionProxyModelPrivate::beginRemoveRows(const QModelIndex& parent, int start, int end) const
{
    Q_Q(const KSelectionProxyModel);
    QPair<int, int> pair = qMakePair(start, end);

    if (m_omitChildren && !m_startWithChildTrees && !m_includeAllSelected) {
        // SubTreeRoots
        if (m_rootIndexList.contains(parent) || isDescendantOf(m_rootIndexList, parent)) {
            return qMakePair(-1, -1);
        }
    }

    const QModelIndex proxyParent = mapParentFromSource(parent);

    if (!m_includeAllSelected && !m_omitChildren) {
        // SubTrees and SubTreesWithoutRoots
        if (proxyParent.isValid()) {
            return pair;
        }
        if (m_startWithChildTrees && m_rootIndexList.contains(parent)) {
            // SubTreesWithoutRoots topLevel
            const int proxyStartRow = getProxyInitialRow(parent) + start;
            return qMakePair(proxyStartRow, proxyStartRow + (end - start));
        }
    }

    if (m_includeAllSelected && m_startWithChildTrees) {
        // ChildrenOfExactSelection
        int position = m_rootIndexList.indexOf(parent);
        if (position != -1) {
            const int proxyStartRow = getProxyInitialRow(parent) + start;
            int proxyEndRow = proxyStartRow + (end - start);
            ++position;
            while (m_rootIndexList.size() < position) {
                const QModelIndex idx = m_rootIndexList.at(position);
                if (isDescendantOf(parent, idx))
                    proxyEndRow += q->sourceModel()->rowCount(idx);
                else
                    break;
            }
            return qMakePair(proxyStartRow, proxyEndRow);
        }
    }

    QList<QPersistentModelIndex>::const_iterator rootIt = m_rootIndexList.constBegin();
    const QList<QPersistentModelIndex>::const_iterator rootEnd = m_rootIndexList.constEnd();
    int rootPosition = 0;
    int rootStartRemove = -1;
    int rootEndRemove = -1;
    int siblingCount = 0;

    for ( ; rootIt != rootEnd; ++rootIt, ++rootPosition) {
        if (m_omitChildren && m_includeAllSelected) {
            // ExactSelection
            if (parent == rootIt->parent() && rootIt->row() <= end && rootIt->row() >= start) {
                if (rootStartRemove == -1)
                    rootStartRemove = rootPosition;
                ++rootEndRemove;
            } else {
                if (rootStartRemove != -1)
                    break;
            }
        } else {
            if (isDescendantOf(parent, *rootIt)) {
                if (rootStartRemove == -1)
                    rootStartRemove = rootPosition;
                ++rootEndRemove;
                if (m_startWithChildTrees)
                    siblingCount += q->sourceModel()->rowCount(*rootIt);
            } else {
                if (rootStartRemove != -1)
                    break;
            }
        }
    }
    if (rootStartRemove != -1) {
        return qMakePair(siblingCount + rootStartRemove, siblingCount + rootEndRemove);
    }

    return qMakePair(-1, -1);
}

void KSelectionProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_Q(KSelectionProxyModel);

    if (!m_selectionModel->hasSelection())
        return;

    QPair<int, int> pair = beginRemoveRows(parent, start, end);
    if (pair.first == -1)
        return;

    const QModelIndex proxyParent = mapParentFromSource(parent);

    m_rowsRemoved = true;
    m_proxyRemoveRows = pair;
    q->beginRemoveRows(proxyParent, pair.first, pair.second);
}

void KSelectionProxyModelPrivate::endRemoveRows(const QModelIndex &sourceParent, int proxyStart, int proxyEnd)
{
    const QModelIndex proxyParent = mapParentFromSource(sourceParent);

    if (proxyParent.isValid())
        updateInternalIndexes(proxyParent, proxyEnd + 1, -1*(proxyEnd - proxyStart + 1));
    else
        updateInternalTopIndexes(proxyEnd + 1, -1*(proxyEnd - proxyStart + 1));

    {
        SourceProxyIndexMapping::right_iterator it = m_mappedParents.rightBegin();

        while (it != m_mappedParents.rightEnd()) {
            if (!it.value().isValid()) {
                m_parentIds.removeRight(it.key());
                it = m_mappedParents.eraseRight(it);
            } else
                ++it;
        }
    }

    {
        // Depending on what is selected at the time, a single removal in the source could invalidate
        // many mapped first child items at once.

        // - A
        // - B
        // - - C
        // - - D
        // - - - E
        // - - - F
        // - - - - G
        // - - - - H

        // If D and F are selected, the proxy contains E, F, G, H. If B is then deleted E to H will
        // be removed, including both first child mappings at E and G.

        SourceProxyIndexMapping::right_iterator it = m_mappedFirstChildren.rightBegin();

        while (it != m_mappedFirstChildren.rightEnd()) {
            if (!it.value().isValid())
                it = m_mappedFirstChildren.eraseRight(it);
            else
                ++it;
        }
    }

    QList<QPersistentModelIndex>::iterator rootIt = m_rootIndexList.begin();
    while (rootIt != m_rootIndexList.end()) {
        if (!rootIt->isValid())
            rootIt = m_rootIndexList.erase(rootIt);
        else
            ++rootIt;
    }
}

void KSelectionProxyModelPrivate::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_Q(KSelectionProxyModel);
    Q_UNUSED(end)

    if (!m_selectionModel->hasSelection())
        return;

    if (!m_rowsRemoved)
        return;
    m_rowsRemoved = false;

    Q_ASSERT(m_proxyRemoveRows.first >= 0);
    Q_ASSERT(m_proxyRemoveRows.second >= 0);
    endRemoveRows(parent, m_proxyRemoveRows.first, m_proxyRemoveRows.second);
    if (m_startWithChildTrees && start == 0 && q->sourceModel()->hasChildren(parent))
        // The private endRemoveRows call might remove the first child mapping for parent, so
        // we create it again in that case.
        createFirstChildMapping(parent, m_proxyRemoveRows.first);

    m_proxyRemoveRows = qMakePair(-1, -1);
    q->endRemoveRows();
}

void KSelectionProxyModelPrivate::sourceRowsAboutToBeMoved(const QModelIndex &srcParent, int srcStart, int srcEnd, const QModelIndex &destParent, int destRow)
{
    Q_Q(KSelectionProxyModel);

    // layout{,AboutToBe}Changed signals are emitted by QAIM for backward compatibility reasons when rows are moved.
    // Processing them is expensive so we do our best to avoid them.
    m_ignoreNextLayoutAboutToBeChanged = true;

    if (!m_selectionModel->hasSelection())
        return;

    const QPair<int, int> removePair = beginRemoveRows(srcParent, srcStart, srcEnd);
    const bool notifyRemove = removePair.first != -1;

    const QModelIndex proxyDestination = q->mapFromSource(destParent);

    // Notify insert if
    // - The destParent is valid or m_startWithChildTrees and destParent is in rootIndexList
    // - Not m_startWithChildTrees and we are moving elements of rootIndexList

    if (!m_startWithChildTrees) {
//         if (m_rootIndexList.contains())
        {


        }
    }

#if 0
    if (m_omitChildren && !m_startWithChildTrees && m_includeAllSelected) {
        // ExactSelection
        int row = srcStart;
        int proxyStart = -1;
        static const int column = 0;
        for ( ; row < srcEnd; ++row) {
            const QModelIndex idx = q->sourceModel()->index(row, column, srcParent);
            proxyStart = m_rootIndexList.indexOf(idx);
            if (proxyStart != -1) {
                break;
            }
        }
        if (proxyStart == -1)
            return;
        const int rangeStart = row;
        row = srcEnd;
        int proxyEnd = proxyStart;
        for ( ; row > rangeStart; --row) {
            const QModelIndex idx = q->sourceModel()->index(row, column, srcParent);
            if (m_rootIndexList.contains(idx) || isDescendantOf(srcParent, idx)) {
                proxyEnd = m_rootIndexList.indexOf(idx);
                break;
            }
        }
        // Figure out if destination is actually a change.
    }
#endif


    bool notifyDestination = proxyDestination.isValid();

    if (proxyDestination.isValid() && (m_omitChildren || (m_startWithChildTrees && m_omitDescendants)))
        notifyDestination = false;

    if (m_includeAllSelected && !m_startWithChildTrees) {
        int destMoveRow = -1;

        QList<QPersistentModelIndex>::const_iterator it = m_rootIndexList.constBegin();
        const QList<QPersistentModelIndex>::const_iterator end = m_rootIndexList.constEnd();

        int startMoveRow = -1;
        int endMoveRow = -1;
        int count = 0;
        int row;
        for ( ; it != end; ++it, ++count) {
            row = it->row();
            if ((row >= srcStart && row <= srcEnd) && q->sourceModel()->parent(*it) == srcParent) {
                if (startMoveRow == -1) {
                    startMoveRow = count;
                    endMoveRow = count;

                    if (destMoveRow == -1) {
                        if (destParent.isValid())
                            destMoveRow = getRootListRow(m_rootIndexList, destParent);
                        else
                            destMoveRow = it->row();
                    }
                } else
                    ++endMoveRow;
            } else {
                if (row < destRow && q->sourceModel()->parent(*it) == destParent) {
                    if (destMoveRow == -1)
                        if (destParent.isValid())
                            destMoveRow = getRootListRow(m_rootIndexList, destParent);
                        else
                            destMoveRow = it->row();
                    else
                        ++destMoveRow;
                }
            }
        }
        if (startMoveRow != -1 && (destMoveRow < startMoveRow || destMoveRow > endMoveRow)) {
            Q_ASSERT(destMoveRow != -1);
            q->beginMoveRows(QModelIndex(), startMoveRow, endMoveRow, QModelIndex(), destMoveRow);
            m_rowsMoved = true;
        }
        return;
    }

    const bool srcInModel = (!m_startWithChildTrees && isInModel(srcParent)) || (m_startWithChildTrees && m_rootIndexList.contains(srcParent));
    const bool destInModel = (!m_startWithChildTrees && isInModel(destParent)) || (m_startWithChildTrees && m_rootIndexList.contains(destParent));

    if (srcInModel) {
        if (destInModel) {
            // The easy case.
            const bool allowMove = q->beginMoveRows(q->mapFromSource(srcParent), srcStart, srcEnd, q->mapFromSource(destParent), destRow);
            Q_UNUSED(allowMove);   // prevent warning in release builds.
            Q_ASSERT(allowMove);
            m_rowsMoved = true;
        } else {
            // source is in the proxy, but dest isn't.
            // Emit a remove
            q->beginRemoveRows(srcParent, srcStart, srcEnd);
        }
    } else {
        if (destInModel) {
            // dest is in proxy, but source is not.
            // Emit an insert
            q->beginInsertRows(destParent, destRow, destRow + (srcEnd - srcStart));
        }
    }
}

void KSelectionProxyModelPrivate::sourceRowsMoved(const QModelIndex &srcParent, int srcStart, int srcEnd, const QModelIndex &destParent, int destRow)
{
    Q_Q(KSelectionProxyModel);

    m_ignoreNextLayoutChanged = true;

    if (!m_selectionModel->hasSelection())
        return;

    if (m_rowsMoved) {
        m_rowsMoved = false;
        endRemoveRows(srcParent, srcStart, srcEnd);
        endInsertRows(destParent, destRow, destRow + (srcEnd - srcStart));
        q->endMoveRows();
        return;
    }

    if (m_rowsRemoved)
        q->endRemoveRows();

    if (m_rowsInserted)
        q->endInsertRows();
}

void KSelectionProxyModelPrivate::removeSelectionFromProxy(const QItemSelection &selection)
{
    Q_Q(KSelectionProxyModel);
    if (selection.isEmpty())
        return;

    q->rootSelectionAboutToBeRemoved(selection);

    foreach(const QItemSelectionRange range, selection)
        removeRangeFromProxy(range);
}

void KSelectionProxyModelPrivate::removeRangeFromProxy(const QItemSelectionRange &range)
{
    Q_Q(KSelectionProxyModel);

    const QModelIndex sourceTopLeft = range.topLeft();
    const QModelIndex proxyTopLeft = q->mapFromSource(sourceTopLeft);
    const QModelIndex sourceBottomLeft = range.bottomRight().sibling(range.bottom(), 0);
    const QModelIndex proxyBottomLeft = q->mapFromSource(sourceBottomLeft);
    const QModelIndex proxyParent = proxyTopLeft.parent();
    const QModelIndex sourceParent = sourceTopLeft.parent();

    if (m_startWithChildTrees) {
        Q_ASSERT(sourceTopLeft.isValid());
        Q_ASSERT(sourceBottomLeft.isValid());
        const int startRootIdx = m_rootIndexList.indexOf(sourceTopLeft);
        int endRootIdx = m_rootIndexList.indexOf(sourceBottomLeft);
        QItemSelection extraRanges;
        if (m_includeAllSelected) {
            // It can happen that indexes of descendants get in between indexes which make up a range.
            // We handle only the first contiguous block here and handle the rest later.
            int idx = startRootIdx;
            const int bottomIdx = endRootIdx;
            const int rootListSize = m_rootIndexList.size();
            int next = idx + 1;
            while (next <= bottomIdx)
            {
                if (next < rootListSize && m_rootIndexList.at(next).parent() == sourceParent) {
                    idx = next;
                    ++next;
                } else
                    break;
            }
            endRootIdx = idx;
            ++idx;
            while (idx <= bottomIdx)
            {
                const QModelIndex index= m_rootIndexList.at(idx);
                if (m_rootIndexList.at(idx).parent() == sourceParent)
                    extraRanges << QItemSelectionRange(index, index);
                ++idx;
            }
        }
        Q_ASSERT(endRootIdx != -1);
        int childrenCount = q->sourceModel()->rowCount(sourceTopLeft);
        for (int rootIdx = startRootIdx + 1; rootIdx <= endRootIdx; ++rootIdx)
        {
            childrenCount += q->sourceModel()->rowCount(m_rootIndexList.at(rootIdx));
        }
        if (childrenCount == 0)
        {
            for (int rootIdx = startRootIdx; rootIdx <= endRootIdx; ++rootIdx)
            {
                const QModelIndex idx = m_rootIndexList.at(startRootIdx);
                q->rootIndexAboutToBeRemoved(idx);
                m_rootIndexList.removeOne(idx);
            }
            return;
        }
        if (!m_includeAllSelected)
        {
            ++endRootIdx;
            for ( ; endRootIdx < m_rootIndexList.size(); ++endRootIdx) {
                const QModelIndex idx = m_rootIndexList.at(endRootIdx);
                if (isDescendantOf(sourceBottomLeft, idx))
                    childrenCount += q->sourceModel()->rowCount(idx);
                else
                    break;
            }
            --endRootIdx;
        }
        const int proxyStart = getTargetRow(startRootIdx);
        int proxyEnd = proxyStart + childrenCount - 1;
        q->beginRemoveRows(QModelIndex(), proxyStart, proxyEnd);

        for (int rootIdx = startRootIdx; rootIdx <= endRootIdx; ++rootIdx)
        {
            q->rootIndexAboutToBeRemoved(m_rootIndexList.at(startRootIdx));
        }

        removeParentMappings(QModelIndex(), proxyStart, proxyEnd);
        removeFirstChildMappings(proxyStart, proxyEnd);
        int numRemovedChildren = 0;
        for (int rootIdx = startRootIdx; rootIdx <= endRootIdx; ++rootIdx)
        {
          const QModelIndex idx = m_rootIndexList.at(startRootIdx);
          const int childCount = q->sourceModel()->rowCount(idx);
          m_rootIndexList.removeAt(startRootIdx);
          numRemovedChildren += childCount;
        }
        updateInternalTopIndexes(proxyEnd + 1, -1 * numRemovedChildren);
        q->endRemoveRows();
        if (m_includeAllSelected) {
            removeSelectionFromProxy(normalizeSelection(extraRanges));
        }
    } else {
        if (!proxyTopLeft.isValid())
            return;
        const int height = range.height();
        q->beginRemoveRows(proxyParent, proxyTopLeft.row(), proxyTopLeft.row() + height - 1);

        // TODO: Do this conditionally if the signal is connected to anything.
        for (int i = 0; i < height; ++i)
        {
            const QModelIndex idx = sourceTopLeft.sibling(range.top() + i, sourceTopLeft.column());
            q->rootIndexAboutToBeRemoved(idx);
        }

        removeParentMappings(proxyParent, proxyTopLeft.row(), proxyTopLeft.row() + height - 1);
        updateInternalIndexes(proxyParent, proxyTopLeft.row() + height, -1 * height);

        for (int i = 0; i < height; ++i)
        {
            const QModelIndex idx = sourceTopLeft.sibling(range.top() + i, sourceTopLeft.column());
            Q_ASSERT(idx.isValid());
            const bool b = m_rootIndexList.removeOne(idx);
            Q_UNUSED(b)
            Q_ASSERT(b);
        }

        q->endRemoveRows();
    }
}

void KSelectionProxyModelPrivate::selectionChanged(const QItemSelection &_selected, const QItemSelection &_deselected)
{
    Q_Q(KSelectionProxyModel);

    if (!q->sourceModel() || (_selected.isEmpty() && _deselected.isEmpty()))
        return;

    // Any deselected indexes in the m_rootIndexList are removed. Then, any
    // indexes in the selected range which are not a descendant of one of the already selected indexes
    // are inserted into the model.
    //
    // All ranges from the selection model need to be split into individual rows. Ranges which are contiguous in
    // the selection model may not be contiguous in the source model if there's a sort filter proxy model in the chain.
    //
    // Some descendants of deselected indexes may still be selected. The ranges in m_selectionModel->selection()
    // are examined. If any of the ranges are descendants of one of the
    // indexes in deselected, they are added to the ranges to be inserted into the model.
    //
    // The new indexes are inserted in sorted order.

    const QItemSelection selected = m_indexMapper->mapSelectionRightToLeft(_selected);
    const QItemSelection deselected = m_indexMapper->mapSelectionRightToLeft(_deselected);

#if QT_VERSION < 0x040800
    // The QItemSelectionModel sometimes doesn't remove deselected items from its selection
    // Fixed in Qt 4.8 : http://qt.gitorious.org/qt/qt/merge_requests/2403
    QItemSelection reportedSelection = m_selectionModel->selection();
    reportedSelection.merge(deselected, QItemSelectionModel::Deselect);
    QItemSelection fullSelection = m_indexMapper->mapSelectionRightToLeft(reportedSelection);
#else
    QItemSelection fullSelection = m_indexMapper->mapSelectionRightToLeft(m_selectionModel->selection());
#endif

    QItemSelection newRootRanges;
    QItemSelection removedRootRanges;
    if (!m_includeAllSelected) {
        newRootRanges = getRootRanges(selected);

        QItemSelection existingSelection = fullSelection;
        // What was selected before the selection was made.
        existingSelection.merge(selected, QItemSelectionModel::Deselect);

        // This is similar to m_rootRanges, but that m_rootRanges at this point still contains the roots
        // of deselected and existingRootRanges does not.

        const QItemSelection existingRootRanges = getRootRanges(existingSelection);
        {
            QMutableListIterator<QItemSelectionRange> i(newRootRanges);
            while (i.hasNext()) {
                const QItemSelectionRange range = i.next();
                const QModelIndex topLeft = range.topLeft();
                if (isDescendantOf(existingRootRanges, topLeft)) {
                    i.remove();
                }
            }
        }

        QItemSelection exposedSelection;
        {
            QItemSelection deselectedRootRanges = getRootRanges(deselected);
            QListIterator<QItemSelectionRange> i(deselectedRootRanges);
            while (i.hasNext()) {
                const QItemSelectionRange range = i.next();
                const QModelIndex topLeft = range.topLeft();
                // Consider this:
                //
                // - A
                // - - B
                // - - - C
                // - - - - D
                //
                // B and D were selected, then B was deselected and C was selected in one go.
                if (!isDescendantOf(existingRootRanges, topLeft)) {
                    // B is topLeft and fullSelection contains D.
                    // B is not a descendant of D.

                    // range is not a descendant of the selection, but maybe the selection is a descendant of range.
                    // no need to check selected here. That's already in newRootRanges.
                    // existingRootRanges and newRootRanges do not overlap.
                    foreach (const QItemSelectionRange &selectedRange, existingRootRanges) {
                        const QModelIndex selectedRangeTopLeft = selectedRange.topLeft();
                        // existingSelection (and selectedRangeTopLeft) is D.
                        // D is a descendant of B, so when B was removed, D might have been exposed as a root.
                        if (isDescendantOf(range, selectedRangeTopLeft)
                                // But D is also a descendant of part of the new selection C, which is already set to be a new root
                                // so D would not be added to exposedSelection because C is in newRootRanges.
                                && !isDescendantOf(newRootRanges, selectedRangeTopLeft))
                            exposedSelection.append(selectedRange);
                    }
                    removedRootRanges << range;
                }
            }
        }

        QItemSelection obscuredRanges;
        {
            QListIterator<QItemSelectionRange> i(existingRootRanges);
            while (i.hasNext()) {
                const QItemSelectionRange range = i.next();
                if (isDescendantOf(newRootRanges, range.topLeft()))
                    obscuredRanges << range;
            }
        }
        removedRootRanges << getRootRanges(obscuredRanges);
        newRootRanges << getRootRanges(exposedSelection);
    } else {
        removedRootRanges = deselected;
        newRootRanges = selected;
    }

    removeSelectionFromProxy(removedRootRanges);

    if (!m_selectionModel->hasSelection())
    {
        Q_ASSERT(m_rootIndexList.isEmpty());
        Q_ASSERT(m_mappedFirstChildren.isEmpty());
        Q_ASSERT(m_mappedParents.isEmpty());
        Q_ASSERT(m_parentIds.isEmpty());
    }

    insertSelectionIntoProxy(newRootRanges);
}

int KSelectionProxyModelPrivate::getTargetRow(int rootListRow)
{
    Q_Q(KSelectionProxyModel);
    if (!m_startWithChildTrees)
        return rootListRow;

    --rootListRow;
    while (rootListRow >= 0) {
        const QModelIndex idx = m_rootIndexList.at(rootListRow);
        Q_ASSERT(idx.isValid());
        const int rowCount = q->sourceModel()->rowCount(idx);
        if (rowCount > 0) {
            static const int column = 0;
            const QModelIndex srcIdx = q->sourceModel()->index(rowCount - 1, column, idx);
            const QModelIndex proxyLastChild = q->mapFromSource(srcIdx);
            return proxyLastChild.row() + 1;
        }
        --rootListRow;
    }
    return 0;
}

void KSelectionProxyModelPrivate::insertSelectionIntoProxy(const QItemSelection &selection)
{
    Q_Q(KSelectionProxyModel);

    if (selection.isEmpty())
        return;

    foreach(const QModelIndex &newIndex, selection.indexes()) {
        if (newIndex.column() > 0)
            continue;
        if (m_startWithChildTrees) {
            const int rootListRow = getRootListRow(m_rootIndexList, newIndex);
            Q_ASSERT(q->sourceModel() == newIndex.model());
            const int rowCount = q->sourceModel()->rowCount(newIndex);
            const int startRow = getTargetRow(rootListRow);

            if (rowCount == 0) {
                // Even if the newindex doesn't have any children to put into the model yet,
                // We still need to make sure it's future children are inserted into the model.
                m_rootIndexList.insert(rootListRow, newIndex);
                if (!m_resetting)
                    emit q->rootIndexAdded(newIndex);
                continue;
            }
            if (!m_resetting)
                q->beginInsertRows(QModelIndex(), startRow, startRow + rowCount - 1);
            Q_ASSERT(newIndex.isValid());
            m_rootIndexList.insert(rootListRow, newIndex);
            emit q->rootIndexAdded(newIndex);

            int _start = 0;
            for (int i = 0; i < rootListRow; ++i)
                _start += q->sourceModel()->rowCount(m_rootIndexList.at(i));

            updateInternalTopIndexes(_start, rowCount);
            createFirstChildMapping(newIndex, _start);
            createParentMappings(newIndex, 0, rowCount - 1);

            if (!m_resetting) {
                q->endInsertRows();
            }

        } else {
            const int row = getRootListRow(m_rootIndexList, newIndex);
            if (!m_resetting)
                q->beginInsertRows(QModelIndex(), row, row);

            Q_ASSERT(newIndex.isValid());
            m_rootIndexList.insert(row, newIndex);
            emit q->rootIndexAdded(newIndex);
            Q_ASSERT(m_rootIndexList.size() > row);
            updateInternalIndexes(QModelIndex(), row, 1);
            createParentMappings(newIndex.parent(), newIndex.row(), newIndex.row());

            if (!m_resetting) {
                q->endInsertRows();
            }
        }
    }
    q->rootSelectionAdded(selection);
}

bool KSelectionProxyModelPrivate::isInModel(const QModelIndex &sourceIndex) const
{
    if (m_rootIndexList.contains(sourceIndex)) {
        if (m_startWithChildTrees)
            return false;
        return true;
    }

    QModelIndex seekIndex = sourceIndex;
    while (seekIndex.isValid()) {
        if (m_rootIndexList.contains(seekIndex)) {
            return true;
        }

        seekIndex = seekIndex.parent();
    }
    return false;
}

KSelectionProxyModel::KSelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent)
        : QAbstractProxyModel(parent), d_ptr(new KSelectionProxyModelPrivate(this, selectionModel))
{
}

KSelectionProxyModel::~KSelectionProxyModel()
{
    delete d_ptr;
}

void KSelectionProxyModel::setFilterBehavior(FilterBehavior behavior)
{
    Q_D(KSelectionProxyModel);

    beginResetModel();

    d->m_filterBehavior = behavior;

    switch (behavior) {
    case SubTrees: {
        d->m_omitChildren = false;
        d->m_omitDescendants = false;
        d->m_startWithChildTrees = false;
        d->m_includeAllSelected = false;
        break;
    }
    case SubTreeRoots: {
        d->m_omitChildren = true;
        d->m_startWithChildTrees = false;
        d->m_includeAllSelected = false;
        break;
    }
    case SubTreesWithoutRoots: {
        d->m_omitChildren = false;
        d->m_omitDescendants = false;
        d->m_startWithChildTrees = true;
        d->m_includeAllSelected = false;
        break;
    }
    case ExactSelection: {
        d->m_omitChildren = true;
        d->m_startWithChildTrees = false;
        d->m_includeAllSelected = true;
        break;
    }
    case ChildrenOfExactSelection: {
        d->m_omitChildren = false;
        d->m_omitDescendants = true;
        d->m_startWithChildTrees = true;
        d->m_includeAllSelected = true;
        break;
    }
    }
    d->resetInternalData();
    d->selectionChanged(d->m_selectionModel->selection(), QItemSelection());

    endResetModel();
}

KSelectionProxyModel::FilterBehavior KSelectionProxyModel::filterBehavior() const
{
    Q_D(const KSelectionProxyModel);
    return (KSelectionProxyModel::FilterBehavior)d->m_filterBehavior;
}

void KSelectionProxyModel::setSourceModel(QAbstractItemModel *_sourceModel)
{
    Q_D(KSelectionProxyModel);
    if (_sourceModel == sourceModel())
        return;

    connect(d->m_selectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));

    beginResetModel();
    d->m_resetting = true;

    if (_sourceModel) {
        disconnect(_sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
                   this, SLOT(sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
        disconnect(_sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
                   this, SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
        disconnect(_sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
                   this, SLOT(sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
        disconnect(_sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
                   this, SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
//     disconnect(_sourceModel, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
//             this, SLOT(sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
//     disconnect(_sourceModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
//             this, SLOT(sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        disconnect(_sourceModel, SIGNAL(modelAboutToBeReset()),
                   this, SLOT(sourceModelAboutToBeReset()));
        disconnect(_sourceModel, SIGNAL(modelReset()),
                   this, SLOT(sourceModelReset()));
        disconnect(_sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                   this, SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex &)));
        disconnect(_sourceModel, SIGNAL(layoutAboutToBeChanged()),
                   this, SLOT(sourceLayoutAboutToBeChanged()));
        disconnect(_sourceModel, SIGNAL(layoutChanged()),
                   this, SLOT(sourceLayoutChanged()));
        disconnect(_sourceModel, SIGNAL(destroyed()),
                   this, SLOT(sourceModelDestroyed()));
    }

    // Must be called before QAbstractProxyModel::setSourceModel because it emits some signals.
    d->resetInternalData();
    QAbstractProxyModel::setSourceModel(_sourceModel);
    if (_sourceModel) {
        d->m_indexMapper = new KModelIndexProxyMapper(_sourceModel, d->m_selectionModel->model(), this);
        if (d->m_selectionModel->hasSelection())
            d->selectionChanged(d->m_selectionModel->selection(), QItemSelection());

        connect(_sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
                SLOT(sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
        connect(_sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
                SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
        connect(_sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
                SLOT(sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
        connect(_sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
                SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
//     connect(_sourceModel, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
//             SLOT(sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
//     connect(_sourceModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
//             SLOT(sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
        connect(_sourceModel, SIGNAL(modelAboutToBeReset()),
                SLOT(sourceModelAboutToBeReset()));
        connect(_sourceModel, SIGNAL(modelReset()),
                SLOT(sourceModelReset()));
        connect(_sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex &)));
        connect(_sourceModel, SIGNAL(layoutAboutToBeChanged()),
                SLOT(sourceLayoutAboutToBeChanged()));
        connect(_sourceModel, SIGNAL(layoutChanged()),
                SLOT(sourceLayoutChanged()));
        connect(_sourceModel, SIGNAL(destroyed()),
                SLOT(sourceModelDestroyed()));
    }

    d->m_resetting = false;
    endResetModel();
}

void KSelectionProxyModelPrivate::updateInternalTopIndexes(int start, int offset)
{
    Q_Q(KSelectionProxyModel);

    updateInternalIndexes(QModelIndex(), start, offset);

    SourceProxyIndexMapping::left_iterator firstChildIt = m_mappedFirstChildren.leftBegin();

    for ( ; firstChildIt != m_mappedFirstChildren.leftEnd(); ++firstChildIt) {
        const QModelIndex proxyIndex = firstChildIt.value();
        Q_ASSERT(proxyIndex.isValid());

        if (proxyIndex.row() < start)
            continue;

        const QModelIndex newProxyIndex = q->createIndex(proxyIndex.row() + offset, proxyIndex.column(), proxyIndex.internalPointer());

        Q_ASSERT(newProxyIndex.isValid());
        m_mappedFirstChildren.updateRight(firstChildIt, newProxyIndex);
    }
}

void KSelectionProxyModelPrivate::updateInternalIndexes(const QModelIndex &parent, int start, int offset)
{
    Q_Q(KSelectionProxyModel);

    Q_ASSERT(start + offset >= 0);

    if (m_omitChildren || (m_omitDescendants && m_startWithChildTrees))
        return;

    SourceProxyIndexMapping::left_iterator mappedParentIt = m_mappedParents.leftBegin();

    QHash<qint64, QModelIndex> updatedParentIds;
    QHash<QPersistentModelIndex, QModelIndex> updatedParents;

    for ( ; mappedParentIt != m_mappedParents.leftEnd(); ++mappedParentIt) {
        const QModelIndex proxyIndex = mappedParentIt.value();
        Q_ASSERT(proxyIndex.isValid());

        if (proxyIndex.row() < start)
            continue;

        const QModelIndex proxyParent = proxyIndex.parent();

        if (parent.isValid()) {
            if (proxyParent != parent)
                continue;
        } else {
            if (proxyParent.isValid())
                continue;
        }
        Q_ASSERT(m_parentIds.rightContains(proxyIndex));
        const qint64 key = m_parentIds.rightToLeft(proxyIndex);

        const QModelIndex newIndex = q->createIndex(proxyIndex.row() + offset, proxyIndex.column(), proxyIndex.internalPointer());

        Q_ASSERT(newIndex.isValid());

        updatedParentIds.insert(key, newIndex);
        updatedParents.insert(mappedParentIt.key(), newIndex);
    }

    {
        QHash<QPersistentModelIndex, QModelIndex>::const_iterator it = updatedParents.constBegin();
        const QHash<QPersistentModelIndex, QModelIndex>::const_iterator end = updatedParents.constEnd();
        for ( ; it != end; ++it)
            m_mappedParents.insert(it.key(), it.value());
    }

    {
        QHash<qint64, QModelIndex>::const_iterator it = updatedParentIds.constBegin();
        const QHash<qint64, QModelIndex>::const_iterator end = updatedParentIds.constEnd();
        for ( ; it != end; ++it)
            m_parentIds.insert(it.key(), it.value());
    }
}

bool KSelectionProxyModelPrivate::parentAlreadyMapped(const QModelIndex &parent) const
{
    return m_mappedParents.leftContains(parent);
}

bool KSelectionProxyModelPrivate::firstChildAlreadyMapped(const QModelIndex &firstChild) const
{
    return m_mappedFirstChildren.leftContains(firstChild);
}

void KSelectionProxyModelPrivate::createFirstChildMapping(const QModelIndex& parent, int proxyRow) const
{
    Q_Q(const KSelectionProxyModel);

    static const int column = 0;
    static const int row = 0;

    const QPersistentModelIndex srcIndex = q->sourceModel()->index(row, column, parent);

    if (firstChildAlreadyMapped(srcIndex))
        return;

    Q_ASSERT(srcIndex.isValid());
    m_mappedFirstChildren.insert(srcIndex, q->createIndex(proxyRow, column));
}

void KSelectionProxyModelPrivate::createParentMappings(const QModelIndex &parent, int start, int end) const
{
    if (m_omitChildren || (m_omitDescendants && m_startWithChildTrees))
        return;

    Q_Q(const KSelectionProxyModel);

    static const int column = 0;

    for (int row = start; row <= end; ++row) {
        const QModelIndex srcIndex = q->sourceModel()->index(row, column, parent);
        Q_ASSERT(srcIndex.isValid());
        if (!q->sourceModel()->hasChildren(srcIndex) || parentAlreadyMapped(srcIndex))
            continue;

        const QModelIndex proxyIndex = q->mapFromSource(srcIndex);
        if (!proxyIndex.isValid())
            return; // If one of them is not mapped, its siblings won't be either

        const qint64 newId = m_nextId++;
        m_parentIds.insert(newId, proxyIndex);
        Q_ASSERT(srcIndex.isValid());
        m_mappedParents.insert(QPersistentModelIndex(srcIndex), proxyIndex);
    }
}

void KSelectionProxyModelPrivate::removeFirstChildMappings(int start, int end)
{
    SourceProxyIndexMapping::left_iterator it = m_mappedFirstChildren.leftBegin();

    while (it != m_mappedFirstChildren.leftEnd()) {
        const int row = it.value().row();
        if (row >= start && row <= end /*&& !parentAlreadyMapped(it->first)*/) {
            it = m_mappedFirstChildren.eraseLeft(it);
        } else
            ++it;
    }
}

void KSelectionProxyModelPrivate::removeParentMappings(const QModelIndex &parent, int start, int end)
{
    Q_Q(KSelectionProxyModel);

    Q_ASSERT(parent.isValid() ? parent.model() == q : true);

    SourceProxyIndexMapping::right_iterator it = m_mappedParents.rightBegin();
    SourceProxyIndexMapping::right_iterator endIt = m_mappedParents.rightEnd();

    typedef QPair<QModelIndex, QPersistentModelIndex> Pair;

    QList<Pair> pairs;

    QModelIndexList list;

    const bool flatList = m_omitChildren || (m_startWithChildTrees && m_omitDescendants);

    while (it != endIt) {
        if (it.key().row() >= start && it.key().row() <= end)
        {
            const QModelIndex sourceParent = it.value();
            const QModelIndex proxyGrandParent = mapParentFromSource(sourceParent.parent());
            if (proxyGrandParent == parent)
            {
                if (!flatList)
                    // Due to recursive calls, we could have several iterators on the container
                    // when erase is called. That's safe accoring to the QHash::iterator docs though.
                    removeParentMappings(it.key(), 0, q->sourceModel()->rowCount(it.value()) - 1);

                m_parentIds.removeRight(it.key());
                it = m_mappedParents.eraseRight(it);
            } else
               ++it;
        } else
           ++it;
    }
}

QModelIndex KSelectionProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    Q_D(const KSelectionProxyModel);

    if (!proxyIndex.isValid() || !sourceModel() || d->m_rootIndexList.isEmpty())
        return QModelIndex();

    Q_ASSERT(proxyIndex.internalId() >= 0);

    if (proxyIndex.internalId() == 0) {
        const int column = proxyIndex.column();
        if (!d->m_startWithChildTrees)
        {
            const QModelIndex idx = d->m_rootIndexList.at(proxyIndex.row());
            return idx.sibling(idx.row(), column);
        }

        if (d->m_mappedFirstChildren.isEmpty())
          return QModelIndex();

        SourceProxyIndexMapping::left_const_iterator it = d->m_mappedFirstChildren.leftConstBegin();
        const SourceProxyIndexMapping::left_const_iterator end = d->m_mappedFirstChildren.leftConstEnd();

        SourceProxyIndexMapping::left_const_iterator result = end;
        for ( ; it != end; ++it) {
            if (it.value().row() <= proxyIndex.row()) {
                if (result == end || it.value().row() > result.value().row())
                    result = it;
            }
        }

        const QModelIndex proxyFirstChild = result.value();
        const QModelIndex sourceFirstChild = result.key();
        Q_ASSERT(proxyFirstChild.isValid());
        Q_ASSERT(proxyFirstChild.internalPointer() == 0);
        Q_ASSERT(sourceFirstChild.isValid());
        return sourceFirstChild.sibling(proxyIndex.row() - proxyFirstChild.row(), column);
    }

    const QModelIndex proxyParent = d->m_parentIds.leftToRight(proxyIndex.internalId());
    Q_ASSERT(proxyParent.isValid());
    const QModelIndex sourceParent = d->mapParentToSource(proxyParent);
    Q_ASSERT(sourceParent.isValid());
    return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), sourceParent);
}

QModelIndex KSelectionProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    Q_D(const KSelectionProxyModel);

    if (!sourceModel() || !sourceIndex.isValid() || d->m_rootIndexList.isEmpty())
        return QModelIndex();

    QModelIndex ancestor = sourceIndex.parent();
    QModelIndexList ancestorList;
    while (ancestor.isValid())
    {
        if (!d->parentAlreadyMapped(ancestor))
            ancestorList.prepend(ancestor);
        else {
            break;
        }
        ancestor = ancestor.parent();
    }
    if (!ancestor.isValid())
        ancestorList.clear();

    // sourceIndex can be mapped to the proxy. We just need to create mappings for its ancestors first.
    for(int i = 0; i < ancestorList.size(); ++i)
    {
        const QModelIndex existingAncestor = d->mapParentFromSource(ancestor);
        Q_ASSERT(existingAncestor.isValid());
        qint64 ansId = d->m_parentIds.rightToLeft(existingAncestor);
        const QModelIndex newSourceParent = ancestorList.at(i);
        const QModelIndex newProxyParent = createIndex(newSourceParent.row(), newSourceParent.column(), reinterpret_cast<void *>(ansId));

        const qint64 newId = d->m_nextId++;
        d->m_parentIds.insert(newId, newProxyParent);
        d->m_mappedParents.insert(QPersistentModelIndex(newSourceParent), newProxyParent);
        ancestor = newSourceParent;
    }


    const QModelIndex maybeMapped = d->mapParentFromSource(sourceIndex);
    if (maybeMapped.isValid()) {
//     Q_ASSERT((!d->m_startWithChildTrees && d->m_rootIndexList.contains(maybeMapped)) ? maybeMapped.row() < 0 : true );
        return maybeMapped;
    }

    const int row = d->m_rootIndexList.indexOf(sourceIndex);
    const QModelIndex sourceParent = sourceIndex.parent();
    if (row != -1) {
        if (!d->m_startWithChildTrees) {
            Q_ASSERT(d->m_rootIndexList.size() > row);
            return createIndex(row, sourceIndex.column());
        }
        if (!d->m_rootIndexList.contains(sourceParent))
            return QModelIndex();

        const QModelIndex firstChild = sourceModel()->index(0, 0, sourceParent);
        const QModelIndex firstProxyChild = d->mapRootFirstChildFromSource(firstChild);

        return createIndex(firstProxyChild.row() + sourceIndex.row(), sourceIndex.column());
    }

    const QModelIndex proxyParent = d->mapParentFromSource(sourceParent);
    if (proxyParent.isValid()) {
        const qint64 parentId = d->m_parentIds.rightToLeft(proxyParent);
        static const int column = 0;
        return createIndex(sourceIndex.row(), column, reinterpret_cast<void *>(parentId));
    }

    // Need rules for breaking the index -> parent -> mapToSource -> mapFromSource cylce.

    if (!d->m_startWithChildTrees)
        return QModelIndex();

    const QModelIndex firstChild = sourceModel()->index(0, 0, sourceParent);
    const QModelIndex firstProxyChild = d->mapRootFirstChildFromSource(firstChild);

    if (!firstProxyChild.isValid())
        return QModelIndex();

    return createIndex(firstProxyChild.row() + sourceIndex.row(), sourceIndex.column());
}

int KSelectionProxyModel::rowCount(const QModelIndex &index) const
{
    Q_D(const KSelectionProxyModel);

    if (!sourceModel() || index.column() > 0 || d->m_rootIndexList.isEmpty())
        return 0;

    if (!index.isValid()) {
        if (!d->m_startWithChildTrees)
            return d->m_rootIndexList.size();

        SourceProxyIndexMapping::left_const_iterator begin = d->m_mappedFirstChildren.leftConstBegin();
        const SourceProxyIndexMapping::left_const_iterator end = d->m_mappedFirstChildren.leftConstEnd();
        if (begin == end)
          return 0;
        const SourceProxyIndexMapping::left_const_iterator result = kMaxElement(begin, end);
        Q_ASSERT(result != end);
        const QModelIndex proxyFirstChild = *result;
        Q_ASSERT(proxyFirstChild.isValid());
        const QModelIndex sourceFirstChild = result.key();
        Q_ASSERT(sourceFirstChild.isValid());
        const QModelIndex sourceParent = sourceFirstChild.parent();
        Q_ASSERT(sourceParent.isValid());
        return sourceModel()->rowCount(sourceParent) + proxyFirstChild.row();
    }

    // index is valid
    if (d->m_omitChildren || (d->m_startWithChildTrees && d->m_omitDescendants))
        return 0;

    QModelIndex sourceParent = d->mapParentToSource(index);

    if (!sourceParent.isValid() && sourceModel()->hasChildren(sourceParent)) {
        sourceParent = mapToSource(index.parent());
        d->createParentMappings(sourceParent, 0, sourceModel()->rowCount(sourceParent) - 1);
        sourceParent = d->mapParentToSource(index);
    }

    if (!sourceParent.isValid())
        return 0;

    return sourceModel()->rowCount(sourceParent);
}

QModelIndex KSelectionProxyModelPrivate::mapParentToSource(const QModelIndex &proxyParent) const
{
    return m_mappedParents.rightToLeft(proxyParent);
}

QModelIndex KSelectionProxyModelPrivate::mapParentFromSource(const QModelIndex &sourceParent) const
{
    return m_mappedParents.leftToRight(sourceParent);
}

QModelIndex KSelectionProxyModelPrivate::mapRootFirstChildToSource(const QModelIndex &proxyChild) const
{
    return m_mappedFirstChildren.rightToLeft(proxyChild);
}

QModelIndex KSelectionProxyModelPrivate::mapRootFirstChildFromSource(const QModelIndex &sourceChild) const
{
    return m_mappedFirstChildren.leftToRight(sourceChild);
}

static bool indexIsValid(bool startWithChildTrees, int row, const QList<QPersistentModelIndex> &rootIndexList, const SourceProxyIndexMapping &mappedFirstChildren)
{
    if (!startWithChildTrees) {
        Q_ASSERT(rootIndexList.size() > row);
    } else {

        Q_ASSERT(!mappedFirstChildren.isEmpty());
        SourceProxyIndexMapping::left_const_iterator it = mappedFirstChildren.leftConstBegin();
        const SourceProxyIndexMapping::left_const_iterator end = mappedFirstChildren.leftConstEnd();

        SourceProxyIndexMapping::left_const_iterator result = end;
        for ( ; it != end; ++it)
        {
            if (it->row() <= row)
            {
                if (result == end || result.value().row() < it.value().row())
                    result = it;
            }
        }
        Q_ASSERT(result != end);
        const QModelIndex proxyFirstChild = result.value();
        const QModelIndex sourceFirstChild = result.key();
        Q_ASSERT(proxyFirstChild.isValid());
        Q_ASSERT(proxyFirstChild.internalPointer() == 0);
        Q_ASSERT(sourceFirstChild.isValid());
        Q_ASSERT(sourceFirstChild.parent().isValid());
        Q_ASSERT(row <= proxyFirstChild.row() + sourceFirstChild.model()->rowCount(sourceFirstChild.parent()));
    }
    return true;
}

QModelIndex KSelectionProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const KSelectionProxyModel);

    if (!sourceModel() || d->m_rootIndexList.isEmpty() || !hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid()) {
        Q_ASSERT(indexIsValid(d->m_startWithChildTrees, row, d->m_rootIndexList, d->m_mappedFirstChildren));
        return createIndex(row, column);
    } else {
        Q_ASSERT(d->m_startWithChildTrees ? true : d->m_parentIds.rightContains(parent));

        const qint64 parentId = d->m_parentIds.rightToLeft(parent);
        return createIndex(row, column, reinterpret_cast<void *>(parentId));
    }
}

QModelIndex KSelectionProxyModel::parent(const QModelIndex &index) const
{
    Q_D(const KSelectionProxyModel);

    if (!sourceModel() || !index.isValid() || d->m_rootIndexList.isEmpty())
        return QModelIndex();

    const qint64 parentId = index.internalId();

    return d->m_parentIds.leftToRight(parentId);
}

Qt::ItemFlags KSelectionProxyModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || !sourceModel())
        return QAbstractProxyModel::flags(index);

    const QModelIndex srcIndex = mapToSource(index);
    Q_ASSERT(srcIndex.isValid());
    return sourceModel()->flags(srcIndex);
}

QVariant KSelectionProxyModel::data(const QModelIndex & index, int role) const
{
    if (!sourceModel())
        return QVariant();

    if (index.isValid()) {
        const QModelIndex idx = mapToSource(index);
        return idx.data(role);
    }
    return sourceModel()->data(index, role);
}

QVariant KSelectionProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!sourceModel())
        return QVariant();
    return sourceModel()->headerData(section, orientation, role);
}

QMimeData* KSelectionProxyModel::mimeData(const QModelIndexList & indexes) const
{
    if (!sourceModel())
        return QAbstractProxyModel::mimeData(indexes);
    QModelIndexList sourceIndexes;
    foreach(const QModelIndex& index, indexes)
        sourceIndexes << mapToSource(index);
    return sourceModel()->mimeData(sourceIndexes);
}

QStringList KSelectionProxyModel::mimeTypes() const
{
    if (!sourceModel())
        return QAbstractProxyModel::mimeTypes();
    return sourceModel()->mimeTypes();
}

Qt::DropActions KSelectionProxyModel::supportedDropActions() const
{
    if (!sourceModel())
        return QAbstractProxyModel::supportedDropActions();
    return sourceModel()->supportedDropActions();
}

bool KSelectionProxyModel::hasChildren(const QModelIndex & parent) const
{
    Q_D(const KSelectionProxyModel);

    if (d->m_rootIndexList.isEmpty() || !sourceModel())
        return false;

    if (parent.isValid()) {
        if (d->m_omitChildren || (d->m_startWithChildTrees && d->m_omitDescendants))
            return false;
        return sourceModel()->hasChildren(mapToSource(parent));
    }

    if (!d->m_startWithChildTrees)
        return true;

    return !d->m_mappedFirstChildren.isEmpty();
}

int KSelectionProxyModel::columnCount(const QModelIndex &index) const
{
    Q_D(const KSelectionProxyModel);
    if (!sourceModel() || index.column() > 0 || d->m_rootIndexList.isEmpty())
        return 0;
    return sourceModel()->columnCount(mapToSource(index));
}

QItemSelectionModel* KSelectionProxyModel::selectionModel() const
{
    Q_D(const KSelectionProxyModel);
    return d->m_selectionModel;
}

bool KSelectionProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_D(const KSelectionProxyModel);
    if (!sourceModel() || d->m_rootIndexList.isEmpty())
        return false;

    if ((row == -1) && (column == -1))
        return sourceModel()->dropMimeData(data, action, -1, -1, mapToSource(parent));

    int source_destination_row = -1;
    int source_destination_column = -1;
    QModelIndex source_parent;

    if (row == rowCount(parent)) {
        source_parent = mapToSource(parent);
        source_destination_row = sourceModel()->rowCount(source_parent);
    } else {
        const QModelIndex proxy_index = index(row, column, parent);
        const QModelIndex source_index = mapToSource(proxy_index);
        source_destination_row = source_index.row();
        source_destination_column = source_index.column();
        source_parent = source_index.parent();
    }
    return sourceModel()->dropMimeData(data, action, source_destination_row,
                                       source_destination_column, source_parent);
}

QList<QPersistentModelIndex> KSelectionProxyModel::sourceRootIndexes() const
{
    Q_D(const KSelectionProxyModel);
    return d->m_rootIndexList;
}

QModelIndexList KSelectionProxyModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    if (role < Qt::UserRole)
        return QAbstractProxyModel::match(start, role, value, hits, flags);

    QModelIndexList list;
    QModelIndex proxyIndex;
    foreach(const QModelIndex &idx, sourceModel()->match(mapToSource(start), role, value, hits, flags)) {
        proxyIndex = mapFromSource(idx);
        if (proxyIndex.isValid())
            list << proxyIndex;

    }
    return list;
}

#include "kselectionproxymodel.moc"
