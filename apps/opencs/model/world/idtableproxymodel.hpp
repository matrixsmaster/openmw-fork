#ifndef CSM_WOLRD_IDTABLEPROXYMODEL_H
#define CSM_WOLRD_IDTABLEPROXYMODEL_H

#include <string>

#include <map>

#include <QSortFilterProxyModel>

#include "columns.hpp"

#include "../../model/world/idtablebase.hpp"

namespace CSMWorld
{
    class IdTableProxyModel : public QSortFilterProxyModel
    {
            Q_OBJECT

            std::string mFilter;

            // Cache of enum values for enum columns(e.g. Modified, Record Type).
            // Used to speed up comparisons during the sort by such columns.
            typedef std::map<Columns::ColumnId, std::vector<std::string> > EnumColumnCache;
            mutable EnumColumnCache mEnumColumnCache;

        protected:

            IdTableBase *mSourceModel;

        public:

            IdTableProxyModel(QObject *parent = 0);

            virtual QModelIndex getModelIndex(const std::string& id, int column) const;

            virtual void setSourceModel(QAbstractItemModel *model);

            void setFilter(std::string filter);

            void refreshFilter();

        protected:

            virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

            virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

            QString getRecordId(int sourceRow) const;

        protected slots:

            virtual void sourceRowsInserted(const QModelIndex &parent, int start, int end);

            virtual void sourceRowsRemoved(const QModelIndex &parent, int start, int end);

            virtual void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

        signals:

            void rowAdded(const std::string &id);
    };
}

#endif
