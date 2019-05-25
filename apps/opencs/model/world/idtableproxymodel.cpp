#include "idtableproxymodel.hpp"

#include <cstdio>
#include <vector>

#include "idtablebase.hpp"

namespace
{
    std::string getEnumValue(const std::vector<std::string> &values, int index)
    {
        if (index < 0 || index >= static_cast<int>(values.size()))
        {
            return "";
        }
        return values[index];
    }
}

bool CSMWorld::IdTableProxyModel::filterAcceptsRow (int sourceRow, const QModelIndex& sourceParent)
    const
{
    Q_ASSERT(mSourceModel != nullptr);

    // It is not possible to use filterAcceptsColumn() and check for
    // sourceModel()->headerData (sourceColumn, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags)
    // because the sourceColumn parameter excludes the hidden columns, i.e. wrong columns can
    // be rejected.  Workaround by disallowing tree branches (nested columns), which are not meant
    // to be visible, from the filter.
    if (sourceParent.isValid())
        return false;

    if (mFilter.empty()) return true;

//    return mFilter->test (*mSourceModel, sourceRow, mColumnMap);

    //TODO: make a test!
    printf("Filter %d = %s\n",sourceRow,mFilter.c_str());
    QString str(mFilter.c_str());

    for (int i = 0; i < 1000; i++) {
        QModelIndex cell = mSourceModel->index(sourceRow,i,sourceParent);
        QVariant data = mSourceModel->data(cell);
        if (!data.isValid()) break;

        printf("%d: Data (%s) is here\n",i,data.typeName());

        if (data.toString().contains(str,Qt::CaseInsensitive)) {
            printf("Returning TRUE\n");
            return true;
        }
    }
    printf("Returning false\n");

    return false;
}

CSMWorld::IdTableProxyModel::IdTableProxyModel (QObject *parent)
    : QSortFilterProxyModel (parent),
      mSourceModel(nullptr)
{
    setSortCaseSensitivity (Qt::CaseInsensitive);
}

QModelIndex CSMWorld::IdTableProxyModel::getModelIndex (const std::string& id, int column) const
{
    Q_ASSERT(mSourceModel != nullptr);

    return mapFromSource(mSourceModel->getModelIndex (id, column));
}

void CSMWorld::IdTableProxyModel::setSourceModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel::setSourceModel(model);

    mSourceModel = dynamic_cast<IdTableBase *>(sourceModel());
    connect(mSourceModel,
            SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this,
            SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
    connect(mSourceModel,
            SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this,
            SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
    connect(mSourceModel,
            SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this,
            SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex &)));
}

void CSMWorld::IdTableProxyModel::setFilter(std::string filter)
{
    beginResetModel();
    mFilter = filter;
    endResetModel();
}

bool CSMWorld::IdTableProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    Columns::ColumnId id = static_cast<Columns::ColumnId>(left.data(ColumnBase::Role_ColumnId).toInt());
    EnumColumnCache::const_iterator valuesIt = mEnumColumnCache.find(id);
    if (valuesIt == mEnumColumnCache.end())
    {
        if (Columns::hasEnums(id))
        {
            valuesIt = mEnumColumnCache.insert(std::make_pair(id, Columns::getEnums(id))).first;
        }
    }

    if (valuesIt != mEnumColumnCache.end())
    {
        std::string first = getEnumValue(valuesIt->second, left.data().toInt());
        std::string second = getEnumValue(valuesIt->second, right.data().toInt());
        return first < second;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

QString CSMWorld::IdTableProxyModel::getRecordId(int sourceRow) const
{
    Q_ASSERT(mSourceModel != nullptr);

    int idColumn = mSourceModel->findColumnIndex(Columns::ColumnId_Id);
    return mSourceModel->data(mSourceModel->index(sourceRow, idColumn)).toString();
}

void CSMWorld::IdTableProxyModel::refreshFilter()
{
    invalidateFilter();
}

void CSMWorld::IdTableProxyModel::sourceRowsInserted(const QModelIndex &parent, int /*start*/, int end)
{
    refreshFilter();
    if (!parent.isValid())
    {
        emit rowAdded(getRecordId(end).toUtf8().constData());
    }
}

void CSMWorld::IdTableProxyModel::sourceRowsRemoved(const QModelIndex &/*parent*/, int /*start*/, int /*end*/)
{
    refreshFilter();
}

void CSMWorld::IdTableProxyModel::sourceDataChanged(const QModelIndex &/*topLeft*/, const QModelIndex &/*bottomRight*/)
{
    refreshFilter();
}
