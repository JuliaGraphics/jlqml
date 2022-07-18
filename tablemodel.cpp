#include "jlcxx/functions.hpp"

#include <QDebug>
#include <QQmlListProperty>
#include "tablemodel.hpp"

namespace qmlwrap
{

TableModel::TableModel(jl_value_t* data, QObject* parent) : QAbstractTableModel(parent)
{
  m_data = new ModelData(data, this);
  // QObject::connect(m_data, &ModelData::countChanged, this, &TableModel::countChanged);
  // QObject::connect(m_data, &ModelData::rolesChanged, this, &TableModel::rolesChanged);
  // QObject::connect(m_data, &ModelData::dataChanged, this, &TableModel::onDataChanged);
}

TableModel::~TableModel()
{
}

int	TableModel::rowCount(const QModelIndex&) const
{
  return m_data->rowcount();
}

int	TableModel::columnCount(const QModelIndex&) const
{
  return m_data->colcount();
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
  return m_data->data(index, role);
}

ModelData* TableModel::get_model_data() const
{
  return m_data;
}

} // namespace qmlwrap
