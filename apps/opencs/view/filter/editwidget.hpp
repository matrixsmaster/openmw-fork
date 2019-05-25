#ifndef CSV_FILTER_EDITWIDGET_H
#define CSV_FILTER_EDITWIDGET_H

#include <QLineEdit>
#include <QPalette>
#include <QtCore/qnamespace.h>

class QModelIndex;

namespace CSMWorld
{
    class Data;
}

namespace CSVFilter
{
    class EditWidget : public QLineEdit
    {
            Q_OBJECT

            bool mIsEmpty;
            int mStateColumnIndex;
            int mDescColumnIndex;

        public:

            EditWidget(CSMWorld::Data& data, QWidget *parent = 0);

            void createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >& filterSource,
                                     Qt::DropAction action);

        signals:

            void filterChanged(std::string filter);

    private:
            std::string generateFilter(std::pair<std::string, std::vector<std::string> >& seekedString) const;

        private slots:

            void textChanged(const QString& text);

            void filterDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void filterRowsRemoved(const QModelIndex& parent, int start, int end);

            void filterRowsInserted(const QModelIndex& parent, int start, int end);


    };
}

#endif
