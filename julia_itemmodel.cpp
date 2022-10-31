#include "julia_itemmodel.hpp"

#include <QVariant>

namespace qmlwrap
{

jl_module_t* JuliaItemModel::m_qml_mod = nullptr;

JuliaItemModel::JuliaItemModel(jl_value_t* data, QObject* parent) : QAbstractTableModel(parent), m_data(data)
{
  assert(m_qml_mod != nullptr);
  jlcxx::protect_from_gc(m_data);
}

JuliaItemModel::~JuliaItemModel()
{
  jlcxx::unprotect_from_gc(m_data);
}

template<typename ReturnT>
auto safe_unbox(jl_value_t* arg)
{
  if(arg == nullptr)
  {
    return ReturnT();
  }
  return jlcxx::unbox<ReturnT>(arg);
}

template<>
auto safe_unbox<QVariant&>(jl_value_t* arg)
{
  if(arg == nullptr)
  {
    return QVariant();
  }
  return jlcxx::unbox<QVariant&>(arg);
}

int JuliaItemModel::rowCount(const QModelIndex& parent) const
{
  static const jlcxx::JuliaFunction rowcount(jl_get_function(m_qml_mod, "rowcount"));
  return safe_unbox<int>(rowcount(m_data));
}

int JuliaItemModel::columnCount(const QModelIndex& parent) const
{
  static const jlcxx::JuliaFunction colcount(jl_get_function(m_qml_mod, "colcount"));
  return safe_unbox<int>(colcount(m_data));
}

QVariant JuliaItemModel::data(const QModelIndex& index, int role) const
{
  static const jlcxx::JuliaFunction data_f(jl_get_function(m_qml_mod, "data"));
  // The static cast avoids sending a reference to the Julia function, which would require adding an extra method
  return safe_unbox<QVariant&>(data_f(m_data, static_cast<int>(role), index.row()+1, index.column()+1));
}

QVariant JuliaItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  static const jlcxx::JuliaFunction headerdata_f(jl_get_function(m_qml_mod, "headerdata"));
  return safe_unbox<QVariant&>(headerdata_f(m_data, section+1, orientation, role));
}

bool JuliaItemModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  static const jlcxx::JuliaFunction setdata(jl_get_function(m_qml_mod, "setdata!"));
  return safe_unbox<bool>(setdata(this, value, static_cast<int>(role), index.row()+1, index.column()+1));
}

bool JuliaItemModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
  static const jlcxx::JuliaFunction setheaderdata_f(jl_get_function(m_qml_mod, "setheaderdata!"));
  return safe_unbox<bool>(setheaderdata_f(this, section+1, orientation, value, role));
}

Qt::ItemFlags JuliaItemModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  if (index.isValid())
    f |= Qt::ItemIsEditable;
  return f;
}

QHash<int, QByteArray> JuliaItemModel::roleNames() const
{
  static const jlcxx::JuliaFunction rolenames(jl_get_function(m_qml_mod, "rolenames"));
  return safe_unbox<QHash<int, QByteArray>>(rolenames(m_data));
}

void JuliaItemModel::clear()
{
  static const jlcxx::JuliaFunction clear_f(jl_get_function(m_qml_mod, "clear!"));
  clear_f(this);
}

void JuliaItemModel::appendRow(const QVariant& row)
{
  static const jlcxx::JuliaFunction append_row_f(jl_get_function(m_qml_mod, "append_row!"));
  append_row_f(this, row);
}

void JuliaItemModel::insertRow(int rowIndex, const QVariant& row)
{
  static const jlcxx::JuliaFunction insert_row_f(jl_get_function(m_qml_mod, "insert_row!"));
  insert_row_f(this, rowIndex+1, row);
}

void JuliaItemModel::moveRow(int fromRowIndex, int toRowIndex, int rows)
{
  static const jlcxx::JuliaFunction move_row_f(jl_get_function(m_qml_mod, "move_rows!"));
  move_row_f(this, fromRowIndex+1, toRowIndex+1, rows);
}

void JuliaItemModel::removeRow(int rowIndex, int rows)
{
  static const jlcxx::JuliaFunction remove_row_f(jl_get_function(m_qml_mod, "remove_rows!"));
  remove_row_f(this, rowIndex+1, rows);
}

void JuliaItemModel::setRow(int rowIndex, const QVariant& row)
{
  static const jlcxx::JuliaFunction set_row_f(jl_get_function(m_qml_mod, "set_row!"));
  set_row_f(this, rowIndex + 1, row);
}

void JuliaItemModel::appendColumn(const QVariant& column)
{
  static const jlcxx::JuliaFunction append_column_f(jl_get_function(m_qml_mod, "append_column!"));
  append_column_f(this, column);
}

void JuliaItemModel::insertColumn(int columnIndex, const QVariant& column)
{
  static const jlcxx::JuliaFunction insert_column_f(jl_get_function(m_qml_mod, "insert_column!"));
  insert_column_f(this, columnIndex+1, column);
}

void JuliaItemModel::moveColumn(int fromColumnIndex, int toColumnIndex, int columns)
{
  static const jlcxx::JuliaFunction move_column_f(jl_get_function(m_qml_mod, "move_columns!"));
  move_column_f(this, fromColumnIndex+1, toColumnIndex+1, columns);
}

void JuliaItemModel::removeColumn(int columnIndex, int columns)
{
  static const jlcxx::JuliaFunction remove_column_f(jl_get_function(m_qml_mod, "remove_columns!"));
  remove_column_f(this, columnIndex+1, columns);
}

void JuliaItemModel::setColumn(int columnIndex, const QVariant& column)
{
  static const jlcxx::JuliaFunction set_column_f(jl_get_function(m_qml_mod, "set_column!"));
  set_column_f(this, columnIndex + 1, column);
}

void JuliaItemModel::emit_data_changed(int startrow, int startcol, int endrow, int endcol)
{
  emit dataChanged(createIndex(startrow-1, startcol-1), createIndex(endrow-1, endcol-1));
}

void JuliaItemModel::emit_header_data_changed(Qt::Orientation orientation, int first, int last)
{
  emit headerDataChanged(orientation, first-1, last-1);
}

void JuliaItemModel::begin_reset_model()
{
  beginResetModel();
}

void JuliaItemModel::end_reset_model()
{
  endResetModel();
}

void JuliaItemModel::begin_insert_rows(int first, int last)
{
  beginInsertRows(QModelIndex(), first-1, last-1);
}

void JuliaItemModel::end_insert_rows()
{
  endInsertRows();
}

bool JuliaItemModel::begin_move_rows(int fromIndex, int toIndex, int count)
{
  fromIndex -= 1;
  toIndex -= 1;
  return beginMoveRows(QModelIndex(), fromIndex, fromIndex+count-1, QModelIndex(), toIndex > fromIndex ? toIndex + count : toIndex);
}

void JuliaItemModel::end_move_rows()
{
  endMoveRows();
}

void JuliaItemModel::begin_remove_rows(int fromIndex, int count)
{
  beginRemoveRows(QModelIndex(), fromIndex-1, fromIndex+count-2);
}

void JuliaItemModel::end_remove_rows()
{
  endRemoveRows();
}

void JuliaItemModel::begin_insert_columns(int first, int last)
{
  beginInsertColumns(QModelIndex(), first-1, last-1);
}

void JuliaItemModel::end_insert_columns()
{
  endInsertColumns();
}

bool JuliaItemModel::begin_move_columns(int fromIndex, int toIndex, int count)
{
  fromIndex -= 1;
  toIndex -= 1;
  return beginMoveColumns(QModelIndex(), fromIndex, fromIndex+count-1, QModelIndex(), toIndex > fromIndex ? toIndex + count : toIndex);
}

void JuliaItemModel::end_move_columns()
{
  endMoveColumns();
}

void JuliaItemModel::begin_remove_columns(int fromIndex, int count)
{
  beginRemoveColumns(QModelIndex(), fromIndex-1, fromIndex+count-2);
}

void JuliaItemModel::end_remove_columns()
{
  endRemoveColumns();
}

QHash<int,QByteArray> JuliaItemModel::default_role_names() const
{
  return QAbstractItemModel::roleNames(); 
}

jl_value_t* JuliaItemModel::get_julia_data() const
{
  return m_data;
}

} // namespace qmlwrap
