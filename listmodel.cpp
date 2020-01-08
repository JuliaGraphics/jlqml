#include "jlcxx/functions.hpp"

#include <QDebug>
#include <QQmlListProperty>
#include "listmodel.hpp"

namespace qmlwrap
{

jl_module_t* ListModel::m_qml_mod = nullptr;

ListModel::ListModel(jl_value_t* data, QObject* parent) : QAbstractListModel(parent), m_data(data)
{
  assert(m_qml_mod != nullptr);
  jlcxx::protect_from_gc(m_data);
}

ListModel::~ListModel()
{
  jlcxx::unprotect_from_gc(m_data);
}

int	ListModel::rowCount(const QModelIndex&) const
{
  return count();
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
  static const jlcxx::JuliaFunction data_f(jl_get_function(m_qml_mod, "data"));
  return jlcxx::unbox<QVariant&>(data_f(m_data, index.row()+1, role+1));
}

QHash<int, QByteArray> ListModel::roleNames() const
{
  QHash<int, QByteArray> result;
  QStringList rolenames = roles();
  for(int i = 0; i != rolenames.size(); ++i)
  {
    result[i] = rolenames[i].toUtf8();
  }

  return result;
}

bool ListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  static const jlcxx::JuliaFunction setdata(jl_get_function(m_qml_mod, "setdata"));
  const bool success = jlcxx::unbox<bool>(setdata(m_data, index.row()+1, value, role+1));
  if(success)
  {
    do_update(index.row(), 1, QVector<int>() << role);
  }
  return success;
}

Qt::ItemFlags ListModel::flags(const QModelIndex&) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void ListModel::append_list(const QVariantList& argvariants)
{
  static const jlcxx::JuliaFunction append_list_f(jl_get_function(m_qml_mod, "append_list"));
  beginInsertRows(QModelIndex(), count(), count());
  append_list_f(m_data, argvariants);
  endInsertRows();
  emit countChanged();
}

void ListModel::append(const QJSValue& record)
{
  if(record.isArray())
  {
    append_list(record.toVariant().toList());
    return;
  }

  QVariantList argvariants;
  QStringList rolenames = roles();
  const int nb_roles = rolenames.size();
  for(int i =0; i != nb_roles; ++i)
  {
    auto rolename = rolenames[i];
    if(record.hasProperty(rolename))
    {
      argvariants.push_back(record.property(QString(rolename)).toVariant());
    }
  }
  append_list(argvariants);
}

Q_INVOKABLE void ListModel::insert(int index, const QJSValue& record)
{
  append(record);
  move(count()-1, index, 1);
}

Q_INVOKABLE void ListModel::insert_list(int index, const QVariantList& argvariants)
{
  append_list(argvariants);
  move(count()-1, index, 1);
}

void ListModel::setProperty(int index, const QString& property, const QVariant& value)
{
  setData(createIndex(index,0), value, roles().indexOf(property));
}

void ListModel::remove(int index)
{
  static const jlcxx::JuliaFunction remove_f(jl_get_function(m_qml_mod, "remove"));

  beginRemoveRows(QModelIndex(), index, index);
  remove_f(m_data, static_cast<int>(index+1));
  endRemoveRows();
  emit countChanged();
}

void ListModel::move(int from, int to, int count)
{
  static const jlcxx::JuliaFunction move_f(jl_get_function(m_qml_mod, "move"));
  if(to == from)
  {
    return;
  }

  if(from < 0 || from >= this->count() || to < 0 || to >= this->count() || to+count > this->count())
  {
    qWarning() << "Invalid indexing for move in ListModel";
    return;
  }

  if(to < from)
  {
    const int c = from - to;
    std::swap(from, to);
    to = from + count;
    count = c;
  }

  beginMoveRows(QModelIndex(), from, from + count - 1, QModelIndex(), to+count);
  move_f(m_data, from+1, to+1, static_cast<int>(count));
  endMoveRows();
}

void ListModel::clear()
{
  static const jlcxx::JuliaFunction clear_f(jl_get_function(m_qml_mod, "clear"));
  beginRemoveRows(QModelIndex(), 0, count());
  clear_f(m_data);
  endRemoveRows();
}

int ListModel::count() const
{
  static const jlcxx::JuliaFunction rowcount(jl_get_function(m_qml_mod, "rowcount"));
  return jlcxx::unbox<int>(rowcount(m_data));
}

void ListModel::emit_roles_changed()
{
  emit rolesChanged();
}

void ListModel::emit_data_changed(int index, int count, const std::vector<int>& roles)
{
  do_update(index, count, QVector<int>::fromStdVector(roles));
}

void ListModel::do_update(int index, int count, const QVector<int> &roles)
{
  emit dataChanged(createIndex(index, 0), createIndex(index + count - 1, 0), roles);
}

QStringList ListModel::roles() const
{
  static const jlcxx::JuliaFunction rolenames(jl_get_function(m_qml_mod, "rolenames"));
  return jlcxx::unbox<QStringList>(rolenames(m_data));
}

void ListModel::push_back(jl_value_t* val)
{
  static const jlcxx::JuliaFunction push("push!");
  beginInsertRows(QModelIndex(), count(), count());
  push(m_data, val);
  endInsertRows();
}

jl_value_t* ListModel::get_julia_data() const
{
  return m_data;
}

} // namespace qmlwrap
