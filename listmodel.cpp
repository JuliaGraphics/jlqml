#include "jlcxx/functions.hpp"

#include <QDebug>
#include <QQmlListProperty>
#include "listmodel.hpp"

namespace qmlwrap
{

jl_function_t* FunctionList::get(const size_t i) const
{
  assert(i < m_functions.size());
  return m_functions[i];
}

void FunctionList::set(const size_t i, jl_function_t* val)
{
  assert(i < m_functions.size());
  protect(val);
  unprotect(m_functions[i]);
  m_functions[i] = val;
}

void FunctionList::clear()
{
  for(jl_function_t* f : m_functions)
  {
    unprotect(f);
  }
  m_functions.clear();
}

void FunctionList::push_back(jl_function_t* f)
{
  protect(f);
  m_functions.push_back(f);
}

void FunctionList::erase(const size_t idx)
{
  assert(idx < m_functions.size());
  unprotect(m_functions[idx]);
  const int nb_functions = size();
  for(int i = idx; i != (nb_functions-1); ++i)
  {
    m_functions[i] = m_functions[i+1];
  }
  m_functions.resize(nb_functions-1);
}

size_t FunctionList::size() const
{
  return m_functions.size();
}

FunctionList::~FunctionList()
{
  clear();
}

void FunctionList::protect(jl_function_t* f)
{
  if(f == nullptr)
  {
    return;
  }
  jlcxx::protect_from_gc(f);
}

void FunctionList::unprotect(jl_function_t* f)
{
  if(f == nullptr)
  {
    return;
  }
  jlcxx::unprotect_from_gc(f);
}

ListModel::ListModel(const jlcxx::ArrayRef<jl_value_t*>& array, jl_function_t* f, QObject* parent) : QAbstractListModel(parent), m_array(array), m_update_array(f)
{
  m_rolenames[0] = "string";
  m_getters.push_back(jlcxx::JuliaFunction("string").pointer());
  m_setters.push_back(nullptr);
  jlcxx::protect_from_gc(m_array.wrapped());
  if(f != nullptr)
  {
    jlcxx::protect_from_gc(f);
  }
}

ListModel::~ListModel()
{
  jlcxx::unprotect_from_gc(m_array.wrapped());
  if(m_update_array != nullptr)
  {
    jlcxx::unprotect_from_gc(m_update_array);
  }
  if(m_constructor != nullptr)
  {
    jlcxx::unprotect_from_gc(m_constructor);
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
  QVariant result = jlcxx::unbox<QVariant>(rolegetter(role)(m_array[index.row()]));
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

  try
  {
    rolesetter(role)((jl_value_t*)m_array.wrapped(), jlcxx::box<QVariant>(value), index.row()+1);
    do_update(index.row(), 1, QVector<int>() << role);
    return true;
  }
  catch(const std::runtime_error&)
  {
    qWarning() << "Null setter for role " << m_rolenames[role] << ", not changing value";
    return false;
  }
}

Qt::ItemFlags ListModel::flags(const QModelIndex&) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void ListModel::append_list(const QVariantList& argvariants)
{
  if(m_constructor == nullptr)
  {
    qWarning() << "No constructor function set, cannot append item to ListModel";
    return;
  }

  const int nb_args = argvariants.size();

  jl_value_t* result = nullptr;
  jl_value_t** julia_args;
  JL_GC_PUSH1(&result);
  JL_GC_PUSHARGS(julia_args, nb_args);
  for(int i = 0; i != nb_args; ++i)
  {
    julia_args[i] = jlcxx::box<QVariant>(argvariants[i]);
  }

  result = jl_call(m_constructor, julia_args, nb_args);
  if(result == nullptr)
  {
    qWarning() << "Error appending ListModel element " << argvariants << ", did you define all required roles for the constructor?";
    JL_GC_POP();
    JL_GC_POP();
    return;
  }

  beginInsertRows(QModelIndex(), m_array.size(), m_array.size());

  m_array.push_back(result);

  do_update();
  JL_GC_POP();
  JL_GC_POP();
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
  const int nb_roles = m_rolenames.size();
  for(int i =0; i != nb_roles; ++i)
  {
    auto rolename = m_rolenames[i];
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
  move(m_array.size()-1, index, 1);
}

Q_INVOKABLE void ListModel::insert_list(int index, const QVariantList& argvariants)
{
  append_list(argvariants);
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
  emit countChanged();
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

  beginMoveRows(QModelIndex(), from, from + count - 1, QModelIndex(), to+count);

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
  endMoveRows();
}

void ListModel::clear()
{
  beginRemoveRows(QModelIndex(), 0, m_array.size());
  jl_array_del_end(m_array.wrapped(), m_array.size());
  do_update();
  endRemoveRows();
}

int ListModel::count() const
{
  return m_array.size();
}

void ListModel::addrole(const std::string& name, jl_function_t* getter, jl_function_t* setter)
{
  if(m_rolenames.values().contains(name.c_str()))
  {
    qWarning() << "Role " << name.c_str() << "exists, aborting add";
    return;
  }

  if(getter == nullptr)
  {
    qWarning() << "Invalid getter for role " << name.c_str() << ", aborting add";
    return;
  }

  if(!m_custom_roles)
  {
    m_rolenames.clear();
    m_getters.clear();
    m_setters.clear();
    m_custom_roles = true;
  }

  std::size_t nb_roles = m_rolenames.size();
  const char* cname = name.c_str();

  std::cout << "adding role " << cname << " at index " << nb_roles << std::endl;

  m_rolenames[nb_roles] = cname;
  m_getters.push_back(getter);
  m_setters.push_back(setter);

  emit rolesChanged();
}

void ListModel::setrole(const int idx, const std::string& name, jl_function_t* getter, jl_function_t* setter)
{
  if(idx >= m_getters.size() || idx < 0)
  {
    qWarning() << "Listmodel index " << idx << " is out of range, aborting setrole";
    return;
  }
  if(m_rolenames.values().contains(name.c_str()) && m_rolenames.key(name.c_str()) != idx)
  {
    qWarning() << "Role " << name.c_str() << "exists, aborting setrole";
    return;
  }

  if(getter == nullptr)
  {
    qWarning() << "Invalid getter for role " << name.c_str() << ", aborting setrole";
    return;
  }

  m_getters.set(idx, getter);
  m_setters.set(idx, setter);
  if(m_rolenames[idx] == name.c_str())
  {
    emit dataChanged(createIndex(0, 0), createIndex(m_array.size() - 1, 0), QVector<int>() << idx);
  }
  else
  {
    m_rolenames[idx] = name.c_str();
    emit rolesChanged();
  }
}

void ListModel::removerole(const int idx)
{
  if(!m_rolenames.contains(idx))
  {
    qWarning() << "Request to delete non-existing role, aborting";
    return;
  }

  const int nb_roles = m_getters.size();
  for(int i = idx; i != (nb_roles-1); ++i)
  {
    m_rolenames[i] = m_rolenames[i+1];
  }
  m_rolenames.remove(nb_roles-1);

  m_getters.erase(idx);
  m_setters.erase(idx);

  emit rolesChanged();
}

void ListModel::removerole(const std::string& name)
{
  if(!m_rolenames.values().contains(name.c_str()))
  {
    qWarning() << "rolename " << name.c_str() << " not found, aborting removerole";
    return;
  }
  removerole(m_rolenames.key(name.c_str()));
}

void ListModel::setconstructor(jl_function_t* constructor)
{
  m_constructor = constructor;
  jlcxx::protect_from_gc(m_constructor);
}

jlcxx::JuliaFunction ListModel::rolegetter(int role) const
{
  if(role < 0 || role >= m_rolenames.size())
  {
    qWarning() << "Role index " << role << " is out of range for ListModel, defaulting to string conversion";
    return jlcxx::JuliaFunction("string");
  }

  return jlcxx::JuliaFunction(m_getters.get(role));
}

jlcxx::JuliaFunction ListModel::rolesetter(int role) const
{
  if(role < 0 || role >= m_rolenames.size())
  {
    qWarning() << "Role index " << role << " is out of range for ListModel, returning null setter";
  }

  return jlcxx::JuliaFunction(m_setters.get(role));
}

void ListModel::do_update(int index, int count, const QVector<int> &roles)
{
  do_update();
  emit dataChanged(createIndex(index, 0), createIndex(index + count - 1, 0), roles);
}

void ListModel::do_update()
{
  if(m_update_array != nullptr)
  {
    jl_call0(m_update_array);
  }
}

QStringList ListModel::roles() const
{
  const int nb_roles = m_rolenames.size();
  QStringList rolelist;
  for(int i = 0; i != nb_roles; ++i)
  {
    rolelist.push_back(m_rolenames[i]);
  }
  return rolelist;
}

void ListModel::push_back(jl_value_t* val)
{
  beginInsertRows(QModelIndex(), m_array.size(), m_array.size());
  m_array.push_back(val);
  do_update();
  endInsertRows();
}

jl_value_t* ListModel::getindex(int i)
{
  return m_array[i-1];
}

void ListModel::setindex(jl_value_t* val, int i)
{
  m_array[i-1] = val;
  
  const int nb_roles = m_rolenames.size();
  QVector<int> roles(nb_roles);
  for(int role_i = 0; role_i != nb_roles; ++role_i)
  {
    roles[role_i] = role_i;
  }
  emit dataChanged(createIndex(i-1, 0), createIndex(i-1, 0), roles);
}

int ListModel::length()
{
  return m_array.size();
}

} // namespace qmlwrap
