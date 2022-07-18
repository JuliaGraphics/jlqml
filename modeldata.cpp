#include "modeldata.hpp"

#include <QVariant>

namespace qmlwrap
{

jl_module_t* ModelData::m_qml_mod = nullptr;

ModelData::ModelData(jl_value_t* data, QAbstractItemModel* parent) : QObject(parent), m_data(data)
{
  assert(m_qml_mod != nullptr);
  jlcxx::protect_from_gc(m_data);
}

ModelData::~ModelData()
{
  jlcxx::unprotect_from_gc(m_data);
}

int ModelData::rowcount() const
{
  static const jlcxx::JuliaFunction rowcount(jl_get_function(m_qml_mod, "rowcount"));
  return jlcxx::unbox<int>(rowcount(m_data));
}

int ModelData::colcount() const
{
  static const jlcxx::JuliaFunction colcount(jl_get_function(m_qml_mod, "colcount"));
  return jlcxx::unbox<int>(colcount(m_data));
}

QVariant ModelData::data(const QModelIndex& index, int role) const
{
  static const jlcxx::JuliaFunction data_f(jl_get_function(m_qml_mod, "data"));
  return jlcxx::unbox<QVariant&>(data_f(m_data, role+1, index.row()+1, index.column()+1));
}

QStringList ModelData::roles() const
{
  static const jlcxx::JuliaFunction rolenames(jl_get_function(m_qml_mod, "rolenames"));
  return jlcxx::unbox<QStringList>(rolenames(m_data));
}

bool ModelData::setData(const QModelIndex& index, const QVariant& value, int role)
{
  static const jlcxx::JuliaFunction setdata(jl_get_function(m_qml_mod, "setdata"));
  const bool success = jlcxx::unbox<bool>(setdata(m_data, value, role+1, index.row()+1, index.column()+1));
  if(success)
  {
    emit_data_changed(index.row(), 1, {role});
  }
  return success;
}

void ModelData::append_list(const QVariantList& argvariants)
{
  static const jlcxx::JuliaFunction append_list_f(jl_get_function(m_qml_mod, "append_list"));
  append_list_f(m_data, argvariants);
}

void ModelData::remove(int index)
{
  static const jlcxx::JuliaFunction remove_f(jl_get_function(m_qml_mod, "remove"));
  remove_f(m_data, static_cast<int>(index+1));
}

ModelData::Move ModelData::normalizemove(int from, int to, int count)
{
  if(to == from)
  {
    return {0,0,0};
  }

  if(from < 0 || from >= this->rowcount() || to < 0 || to >= this->rowcount() || to+count > this->rowcount())
  {
    return {0,0,-1}; // count -1 to indicate invalid move
  }

  if(to < from)
  {
    const int c = from - to;
    std::swap(from, to);
    to = from + count;
    count = c;
  }

  return {from,to,count};
}

void ModelData::move(const Move& m)
{
  static const jlcxx::JuliaFunction move_f(jl_get_function(m_qml_mod, "move"));
  if(m.count == 0)
  {
    return;
  }

  if(m.from > m.to || m.count < 0)
  {
    qWarning() << "Invalid indexing for move from " << m.from << " to " << m.to;
  }

  move_f(m_data, m.from+1, m.to+1, m.count);
}

void ModelData::clear()
{
  static const jlcxx::JuliaFunction clear_f(jl_get_function(m_qml_mod, "clear"));
  clear_f(m_data);
}

void ModelData::emit_roles_changed()
{
  emit rolesChanged();
}

void ModelData::emit_data_changed(int index, int count, const std::vector<int>& roles)
{
  emit dataChanged(index, count, roles);
}


void ModelData::push_back(jl_value_t* val)
{
  static const jlcxx::JuliaFunction push("push!");
  push(m_data, val);
}

jl_value_t* ModelData::get_julia_data() const
{
  return m_data;
}

} // namespace qmlwrap
