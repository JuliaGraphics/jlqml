#ifndef QML_TABLEMODEL_H
#define QML_TABLEMODEL_H

#include <map>
#include <string>

#include <QAbstractTableModel>
#include <QJSValue>
#include <QObject>

#include "modeldata.hpp"

namespace qmlwrap
{

/// Wrap Julia composite types
class TableModel : public QAbstractTableModel
{
  Q_OBJECT
public:

  TableModel(jl_value_t* data, QObject* parent = 0);
  virtual ~TableModel();

  // QAbstractItemModel interface
  virtual int	rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int	columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  
  ModelData* get_model_data() const;

private:

  ModelData* m_data;
};

}

#endif
