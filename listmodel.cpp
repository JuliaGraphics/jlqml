#include <functions.hpp>

#include <QDebug>
#include <QQmlListProperty>
#include "listmodel.hpp"

namespace qmlwrap
{

ListModel::ListModel(const cxx_wrap::ArrayRef<jl_value_t*>& array, jl_function_t* f, QObject* parent) : QAbstractListModel(parent), m_array(array), m_update_array(f)
{
  m_rolenames[0] = "string";
  cxx_wrap::protect_from_gc(array.wrapped());
  if(f != nullptr)
  {
    cxx_wrap::protect_from_gc(f);
  }
}

ListModel::~ListModel()
{
  cxx_wrap::unprotect_from_gc(m_array.wrapped());
  if(m_update_array != nullptr)
  {
    cxx_wrap::unprotect_from_gc(m_update_array);
  }
}

int	ListModel::rowCount(const QModelIndex& parent) const
{
  return m_array.size();
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
  if(index.row() < 0 || index.row() >= m_array.size())
  {
    qWarning() << "Row index " << index << " is out of range for ListModel";
    return QVariant();
  }
  QVariant result = cxx_wrap::convert_to_cpp<QVariant>(cxx_wrap::julia_call(rolefunction(role), m_array[index.row()]));
  return result;
}

QHash<int, QByteArray> ListModel::roleNames() const
{
  return m_rolenames;
}

bool ListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if(index.row() < 0 || index.row() >= m_array.size())
  {
    qWarning() << "Row index " << index << " is out of range for ListModel";
    return false;
  }

  jl_value_t* result = cxx_wrap::julia_call(rolefunction(role), cxx_wrap::convert_to_julia(value));
  if(result != nullptr)
  {
    jl_arrayset(m_array.wrapped(), result, index.row());
    do_update();
    return true;
  }

  return false;
}

Qt::ItemFlags ListModel::flags(const QModelIndex&) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void ListModel::append(const QVariantList& argvariants)
{
  if(m_constructor == nullptr)
  {
    qWarning() << "No constructor function set, cannot append item to ListModel";
    return;
  }

  beginInsertRows(QModelIndex(), m_array.size(), m_array.size());

  const int nb_args = argvariants.size();

  jl_value_t* result = nullptr;
  jl_value_t** julia_args;
  JL_GC_PUSH1(&result);
  JL_GC_PUSHARGS(julia_args, nb_args);
  for(int i = 0; i != nb_args; ++i)
  {
    julia_args[i] = cxx_wrap::convert_to_julia(argvariants[i]);
  }

  m_array.push_back(jl_call(m_constructor, julia_args, nb_args));

  JL_GC_POP();
  JL_GC_POP();
  do_update();
  endInsertRows();
}

void ListModel::append(const QJSValue& record)
{
  QVariantList argvariants;
  for(const auto& rolename : m_rolenames)
  {
    if(record.hasProperty(rolename))
    {
      argvariants.push_back(record.property(QString(rolename)).toVariant());
    }
  }
  append(argvariants);
}

Q_INVOKABLE void ListModel::insert(int index, const QJSValue& record)
{
  append(record);
  move(m_array.size()-1, index, 1);
}

Q_INVOKABLE void ListModel::insert(int index, const QVariantList& argvariants)
{
  append(argvariants);
  move(m_array.size()-1, index, 1);
}

void ListModel::setProperty(int index, const QString& property, const QVariant& value)
{
  setData(createIndex(index,0), value, m_rolenames.key(property.toUtf8()));
}

void ListModel::remove(int index)
{
  if(index < 0 || index >= m_array.size())
  {
    qWarning() << "Row index " << index << " is out of range for ListModel";
    return;
  }

  beginRemoveRows(QModelIndex(), index, index);

  int nb_elems = m_array.size();
  for(int i = index; i != nb_elems-1; ++i)
  {
    m_array[i] = m_array[i+1];
  }

  jl_array_del_end(m_array.wrapped(), 1);

  do_update();

  endRemoveRows();
}

void ListModel::move(int from, int to, int count)
{
  if(from == to || count == 0)
    return;

  if(to < from)
  {
    const int c = from - to;
    std::swap(from, to);
    to = from + count;
    count = c;
  }

  if(from < 0 || from >= m_array.size() || to < 0 || to >= m_array.size() || to+count > m_array.size())
  {
    qWarning() << "Invalid indexing for move in ListModel";
    return;
  }

  jl_value_t** removed_elems;
  JL_GC_PUSHARGS(removed_elems, count);

  // Save elements to move
  for(int i = 0; i != count; ++i)
  {
    removed_elems[i] = m_array[from+i];
  }
  // Shift elements that remain
  for(int i = from; i != to; ++i)
  {
    m_array[i] = m_array[i+count];
  }
  // Place saved elements in to block
  for(int i = 0; i != count; ++i)
  {
    m_array[to+i] = removed_elems[i];
  }

  do_update();

  JL_GC_POP();
}

void ListModel::clear()
{
  jl_array_del_end(m_array.wrapped(), m_array.size());
  do_update();
}

void ListModel::setrolenames(cxx_wrap::ArrayRef<std::string> names)
{
  m_rolenames.clear();
  const int nb_names = names.size();
  for(int i = 0; i != nb_names; ++i)
  {
    m_rolenames[i] = names[i].c_str();
  }
}

void ListModel::setconstructor(const std::string& constructor)
{
  m_constructor = cxx_wrap::julia_function(constructor);
}

jl_function_t* ListModel::rolefunction(int role) const
{
  jl_function_t* role_function = cxx_wrap::julia_function("string");
  if(role < 0 || role >= m_rolenames.size())
  {
    qWarning() << "Role index " << role << " is out of range for ListModel, defaulting to string conversion";
    return role_function;
  }

  try
  {
    role_function = cxx_wrap::julia_function(m_rolenames[role].toStdString());
  }
  catch(const std::runtime_error&)
  {
    qWarning() << "Failed to find function " << m_rolenames[role] << ", defaulting to string conversion";
  }

  return role_function;
}

void ListModel::do_update()
{
  if(m_update_array != nullptr)
  {
    jl_call0(m_update_array);
  }
}

} // namespace qmlwrap
